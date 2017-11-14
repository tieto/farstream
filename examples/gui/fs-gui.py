#!/usr/bin/python

# Farstream demo GUI program
#
# Copyright (C) 2007 Collabora, Nokia
# @author: Olivier Crete <olivier.crete@collabora.co.uk>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
#

import sys, os, pwd, os.path
import socket
import threading
import weakref

import gc

import signal

import gi
gi.require_version('Farstream', '0.2')
gi.require_version('Gst', '1.0')
gi.require_version('GstVideo', '1.0')
gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import GLib, GObject, Gtk, Gdk, GdkX11
from gi.repository import Farstream, Gst, GstVideo

from fs_gui_net import FsUIClient, FsUIListener, FsUIServer

CAMERA=False

AUDIO=True
VIDEO=True

CLIENT=1
SERVER=2

TRANSMITTER="nice"

INITIAL_VOLUME=50.0

builderprefix = os.path.join(os.path.dirname(__file__),"fs-gui-")


def make_video_sink(pipeline, xid, name, sync=True):
    "Make a bin with a video sink in it, that will be displayed on xid."
    bin = Gst.parse_bin_from_description("videoconvert ! videoscale ! videoconvert ! xvimagesink", True)
    sink = bin.get_by_interface(GstVideo.VideoOverlay)
    assert sink
    bin.set_name("videosink_%d" % xid)
    sink.set_window_handle(xid)
    sink.props.sync = sync
    return bin


class FsUIPipeline (GObject.Object):
    "Object to wrap the GstPipeline"

    level = GObject.Property(type=float)

    def int_handler(self, sig, frame):
        try:
            Gst.DEBUG_BIN_TO_DOT_FILE(self.pipeline, 0, "pipelinedump")
        except:
            pass
        sys.exit(2)

    def g_int_handler(self):
        self.int_handler(None, None)
    
    def __init__(self, elementname="fsrtpconference"):
        GObject.GObject.__init__(self)
        self.pipeline = Gst.Pipeline()
        GLib.unix_signal_add_full(0, signal.SIGINT, self.g_int_handler, None,)
        signal.signal(signal.SIGINT, self.int_handler)
        #self.pipeline.get_bus().set_sync_handler(self.sync_handler, None)
        self.pipeline.get_bus().add_watch(0, self.async_handler, None)
        self.conf = Gst.ElementFactory.make(elementname, None)
        self.notifier = Farstream.ElementAddedNotifier()
        self.notifier.add(self.pipeline)
        self.notifier.set_default_properties(self.conf)

        self.pipeline.add(self.conf)
        if VIDEO:
            self.videosource = FsUIVideoSource(self.pipeline)
            self.videosession = FsUISession(self.conf, self.videosource)
        if AUDIO:
            self.audiosource = FsUIAudioSource(self.pipeline)
            self.audiosession = FsUISession(self.conf, self.audiosource)
            #self.adder = None
        self.pipeline.set_state(Gst.State.PLAYING)

    def __del__(self):
        self.pipeline.set_state(Gst.State.NULL)

    def set_mute(self, muted):
        if AUDIO:
            self.audiosource.volume.props.mute = muted

    def set_volume(self, volume):
        if AUDIO:
            self.audiosource.volume.props.volume = volume / 100

    def sync_handler(self, bus, message, data):
        "Message handler to get the prepare-xwindow-id event"
        if message.type == Gst.MessageType.ELEMENT and \
               message.get_structure().has_name("prepare-xwindow-id"):
            xid = None
            element = message.src
            # We stored the XID on the element or its parent on the expose event
            # Now lets look it up
            while not xid and element:
                xid = element.xid
                element = element.get_parent()
            if xid:
                message.src.set_window_handle(xid)
                return Gst.BusSyncReply.DROP
        return Gst.BusSyncReply.PASS

    def async_handler(self, bus, message, Data):
        "Async handler to print messages"
        if message.type == Gst.MessageType.ERROR:
            print "ERROR: ", message.src.get_name(), ": ", message.parse_error()
            sys.exit(1)
        elif message.type == Gst.MessageType.WARNING:
            print "WARNING: ", message.src.get_name(), ": ", message.parse_warning()
        elif message.type == Gst.MessageType.LATENCY:
            self.pipeline.recalculate_latency()
        elif message.type == Gst.MessageType.ELEMENT:
            sessions = []
            if AUDIO:
                sessions += [self.audiosession]
            if VIDEO:
                sessions += [self.videosession]
            for session in sessions:
                (changed, codec, sec_codecs) = session.fssession.parse_send_codec_changed(message)
                if changed:
                    print "Send codec changed: " + codec.to_string()
                if changed or session.fssession.parse_codecs_changed(message):
                    print "Codecs changed"

                    if self.audiosession == session:
                        self.codecs_changed_audio()
                    if self.videosession == session:
                        self.codecs_changed_video()
                    return True

                for stream in session.streams:
                    (res, candidate) = stream.fsstream.parse_new_local_candidate(message)
                    if res:
                        print "New Local candidate"
                        stream.new_local_candidate(candidate)
                        return True

                    (res, codecs) = stream.fsstream.parse_recv_codecs_changed(message)
                    if res:
                        print "Receive codecs changed"
                        stream.recv_codecs_changed(codecs)
                        return True

                    if stream.fsstream.parse_local_candidates_prepared(message):
                        print "Local candidates prepared"
                        stream.local_candidates_prepared()
                        return True

            if message.get_structure().has_name("dtmf-event"):
                print "dtmf-event: %d" % message.get_structure().get_int("number")

            elif message.get_structure().has_name("farstream-error"):
                print "Async error ("+ str(message.get_structure().get_int("error-no")) +"): " + message.get_structure().get_string("error-msg")
            elif message.get_structure().has_name("level"):
                decay = message.get_structure().get_value("decay")
                rms = message.get_structure().get_value("rms")
                avg_rms = float(sum(rms))/len(rms) if len(rms) > 0 else float('nan')
                avg_linear = pow (10, avg_rms / 20) * 100
                if message.src == self.audiosource.level:
                    self.props.level = avg_linear
                else:
                    for stream in self.audiosession.streams:
                        for sink in stream.sinks:
                            level = sink.get_by_name("l")
                            if level and level is message.src:
                                stream.participant.props.level = avg_linear
                                break
            else:
                print message.src.get_name(), ": ", message.get_structure().get_name()
        elif message.type != Gst.MessageType.STATE_CHANGED \
                 and message.type != Gst.MessageType.ASYNC_DONE \
                 and message.type != Gst.MessageType.QOS:
            print message.type
        return True

    def make_video_preview(self, xid, newsize_callback):
        "Creates the preview sink"
        self.previewsink = make_video_sink(self.pipeline, xid,
                                           "previewvideosink", False)
        self.pipeline.add(self.previewsink)
        #Add a probe to wait for the first buffer to find the image size
        self.havesize = self.previewsink.get_static_pad("sink").add_probe(Gst.PadProbeType.EVENT_DOWNSTREAM, self.have_size, newsize_callback)
                                                          
        self.previewsink.set_state(Gst.State.PLAYING)
        self.videosource.tee.link(self.previewsink)
        return self.previewsink

    def have_size(self, pad, info, callback):
        "Callback on the first buffer to know the drawingarea size"
        if (info.get_event().type != Gst.EventType.CAPS):
            return Gst.PadProbeReturn.OK
        
        caps = info.get_event().parse_caps()
        x = caps.get_structure(0).get_int("width")[1]
        y = caps.get_structure(0).get_int("height")[1]
        callback(x,y)
        return Gst.PadProbeReturn.OK

    def link_audio_sink(self, pad, sinkcount):
        "Link the audio sink to the pad"
        # print >>sys.stderr, "LINKING AUDIO SINK"
        # if not self.adder:
        #     audiosink = Gst.ElementFactory.make("alsasink", None)
        #     audiosink.set_property("buffer-time", 50000)
        #     self.pipeline.add(audiosink)

        #     try:
        #         self.adder = Gst.ElementFactory.make("liveadder", None)
        #     except Gst.ElementNotFoundError:
        #         audiosink.set_state(Gst.State.PLAYING)
        #         pad.link(audiosink.get_static_pad("sink"))
        #         return
        #     self.pipeline.add(self.adder)
        #     audiosink.set_state(Gst.State.PLAYING)
        #     self.adder.link(audiosink)
        #     self.adder.set_state(Gst.State.PLAYING)
        # convert1 = Gst.ElementFactory.make("audioconvert", None)
        # self.pipeline.add(convert1)
        # resample = Gst.ElementFactory.make("audioresample", None)
        # self.pipeline.add(resample)
        # convert2 = Gst.ElementFactory.make("audioconvert", None)
        # self.pipeline.add(convert2)
        # convert1.link(resample)
        # resample.link(convert2)
        # convert2.link(self.adder)
        # pad.link(convert1.get_static_pad("sink"))
        # convert2.set_state(Gst.State.PLAYING)
        # resample.set_state(Gst.State.PLAYING)
        # convert1.set_state(Gst.State.PLAYING)

        sink = Gst.parse_bin_from_description("level name=l ! pulsesink name=v", True)
        sink.set_name("audiosink_" + str(sinkcount));
        print sink
        print sink.get_name()
        self.pipeline.add(sink)
        sink.set_state(Gst.State.PLAYING)
        if pad.link(sink.get_static_pad("sink")) != Gst.PadLinkReturn.OK:
            print("LINK FAILED")
        self.pipeline.post_message(Gst.Message.new_latency(sink))
        return sink

class FsUISource:
    "An abstract generic class for media sources"

    def __init__(self, pipeline):
        self.pipeline = pipeline
        self.tee = Gst.ElementFactory.make("tee", None)
        pipeline.add(self.tee)
        self.fakesink = Gst.ElementFactory.make("fakesink", None)
        self.fakesink.props.async = False
        self.fakesink.props.sync = False
        self.fakesink.set_state(Gst.State.PLAYING)
        self.tee.set_state(Gst.State.PLAYING)
        pipeline.add(self.fakesink)
        self.tee.link(self.fakesink)

        self.source = self.make_source()
        pipeline.add(self.source)
        self.source.link(self.tee)

    def __del__(self):
        self.source.set_state(Gst.State.NULL)
        self.tee.set_state(Gst.State.NULL)
        self.pipeline.remove(self.source)
        self.pipeline.remove(self.tee)
        
        
    def make_source(self):
        "Creates and returns the source GstElement"
        raise NotImplementedError()


    def get_type(self):
        "Returns the FsMediaType of the source."
        raise NotImplementedError()

    def get_src_pad(self, name="src_%u"):
        "Gets a source pad from the source"
        requestpad = self.tee.get_request_pad(name)
        assert requestpad        
        return requestpad

    def put_src_pad(self, pad):
        "Puts the source pad from the source"
        self.tee.release_request_pad(pad)
    

class FsUIVideoSource(FsUISource):
    "A Video source"
    
    def get_type(self):
        return Farstream.MediaType.VIDEO

    def make_source(self):
        bin = Gst.Bin()
        if CAMERA:
            source = Gst.ElementFactory.make("v4l2src", None)
            source.set_property("device", CAMERA)
            bin.add(source)
        else:
            source = Gst.ElementFactory.make("videotestsrc", None)
            source.set_property("is-live", 1)
            bin.add(source)
            overlay = Gst.ElementFactory.make("timeoverlay", None)
            overlay.set_property("font-desc", "Sans 32")
            bin.add(overlay)
            source.link(overlay)
            source=overlay

        filter = Gst.ElementFactory.make("capsfilter", None)
        filter.set_property("caps", Gst.Caps.from_string("video/x-raw , width=[300,500] , height=[200,500], framerate=[20/1,30/1]"))
        bin.add(filter)
        source.link(filter)

        videoscale = Gst.ElementFactory.make("videoscale", None)
        bin.add(videoscale)
        filter.link(videoscale)

        bin.add_pad(Gst.GhostPad.new("src", videoscale.get_static_pad("src")))
        return bin
            
      

class FsUIAudioSource(FsUISource):
    "An audio source"

    def get_type(self):
        return Farstream.MediaType.AUDIO
    
    def make_source(self):
        AUDIOSOURCE = "audiotestsrc is-live=1 wave=1  ! volume name=v"
        #AUDIOSOURCE = "pulsesrc name=v"
        source = Gst.parse_bin_from_description(AUDIOSOURCE + " volume=" + str(INITIAL_VOLUME/100) + " ! level message=1 name=l", True)
        self.volume = source.get_by_name("v")
        self.level = source.get_by_name("l")
        return source

class FsUISession:
    "This is one session (audio or video depending on the source)"
    
    def __init__(self, conference, source):
        self.conference = conference
        self.source = source
        self.streams = weakref.WeakSet()
        self.fssession = conference.new_session(source.get_type())
        self.fssession.uisession = self
        self.fssession.set_codec_preferences(
            Farstream.utils_get_default_codec_preferences(conference))

        self.queue = Gst.ElementFactory.make("queue", None)
        self.source.pipeline.add(self.queue)

        self.sourcepad = self.source.get_src_pad()
        self.queue.get_static_pad("src").link(self.fssession.get_property("sink-pad"))
        self.queue.set_state(Gst.State.PLAYING)
        self.sourcepad.link(self.queue.get_static_pad("sink"))

    def __del__(self):
        self.sourcepad(unlink)
        self.source.put_src_pad(self.sourcepad)
        self.source.pipeline.remove(self.queue)
        self.queue.set_state(Gst.State.NULL)
            
    def new_stream(self, id, participant):
        "Creates a new stream for a specific participant"
        transmitter_params = {}
        # If its video, we start at port 9078, to make it more easy
        # to differentiate it in a tcpdump log
        if self.source.get_type() == Farstream.MediaType.VIDEO and \
               TRANSMITTER == "rawudp":
            cand = Farstream.Candidate.new("",Farstream.ComponentType.RTP,
                                           Farstream.CandidateType.HOST,
                                           Farstream.NetworkProtocol.UDP,
                                           None, 9078)
            value = GObject.Value()
            Farstream.value_set_candidate_list(value, [cand])
            transmitter_params["preferred-local-candidates"] = value
        realstream = self.fssession.new_stream(participant.fsparticipant,
                                               Farstream.StreamDirection.BOTH)
        print TRANSMITTER + ": " + str(transmitter_params)
        realstream.set_transmitter_ht(TRANSMITTER, transmitter_params)
        stream = FsUIStream(id, self, participant, realstream)
        self.streams.add(stream)
        return stream

    def dtmf_start(self, event):
        if (event == "*"):
            event = Farstream.DTMFEvent.STAR
        elif (event == "#"):
            event = Farstream.DTMFEvent.POUND
        else:
            event = int(event)
        self.fssession.start_telephony_event(event, 2)
        
    def dtmf_stop(self):
        self.fssession.stop_telephony_event()

    def codecs_changed(self):
        "Callback from FsSession"
        for s in self.streams:
            s.codecs_changed()

    def send_stream_codecs(self, codecs, sourcestream):
        for s in self.streams:
            if s is not sourcestream:
                s.connect.send_codecs(s.participant.id,
                                      sourcestream.id,
                                      codecs,
                                      sourcestream.participant.id)

class FsUIStream:
    "One participant in one session"

    def __init__(self, id, session, participant, fsstream):
        self.id = id
        self.session = session
        self.participant = participant
        self.fsstream = fsstream
        self.connect = participant.connect
        self.fsstream.uistream = self
        self.fsstream.connect("src-pad-added", self.__src_pad_added)
        self.send_codecs = False
        self.last_codecs = None
        self.last_stream_codecs = None
        self.candidates = []
        self.sinks = []
        self.sinkcount = 0

    def local_candidates_prepared(self):
        "Callback from FsStream"
        self.connect.send_candidates_done(self.participant.id, self.id)
    def new_local_candidate(self, candidate):
        "Callback from FsStream"
        if "." in candidate.ip:
            print "IPv4 Candidate: %s %s" % (candidate.ip, candidate.port)
        elif ":" in candidate.ip:
            print "IPv6 Candidate: %s %s" % (candidate.ip, candidate.port)
        else:
            print "STRANGE Candidate: " + candidate.ip
        self.connect.send_candidate(self.participant.id, self.id, candidate)
    def __src_pad_added(self, stream, pad, codec):
        "Callback from FsStream"
        if self.session.source.get_type() == Farstream.MediaType.VIDEO:
            self.participant.link_video_sink(pad)
        else:
            sink = self.participant.pipeline.link_audio_sink(pad,
                                                             self.sinkcount)
            self.sinkcount += 1
            v = sink.get_by_name("v")
            self.participant.volume_adjustment.connect("value-changed",
                                                       lambda adj, nonevalue:
                                                           v.set_property("volume",float(adj.props.value) / 100), None)
            self.participant.mute_button.bind_property("active", v, "mute",
                                                       GObject.BindingFlags.SYNC_CREATE)
            self.sinks.append(sink)

    def candidate(self, candidate):
        "Callback for the network object."
        self.candidates.append(candidate)
    def candidates_done(self):
        "Callback for the network object."
        if TRANSMITTER == "rawudp":
            self.fsstream.force_remote_candidates(self.candidates)
        else:
            self.fsstream.add_remote_candidates(self.candidates)
        self.candidates = []
    def codecs(self, codecs):
        "Callback for the network object. Set the codecs"

        print "Remote codecs"
        for c in codecs:
            print "Got remote codec from %s/%s %s" % \
                  (self.participant.id, self.id, c.to_string())
        oldcodecs = self.fsstream.props.remote_codecs
        if Farstream.codec_list_are_equal(oldcodecs,codecs):
            return
        try:
            self.fsstream.set_remote_codecs(codecs)
        except AttributeError:
            print "Tried to set codecs with 0 codec"
        self.send_local_codecs()
        self.send_stream_codecs()


    def send_local_codecs(self):
        "Callback for the network object."
        print "send_local_codecs"
        self.send_codecs = True
        self.check_send_local_codecs()

    def codecs_changed(self):
        print "codecs changed"
        self.check_send_local_codecs()
        self.send_stream_codecs()

    def check_send_local_codecs(self):
        "Internal function to send our local codecs when they're ready"
        print "check_send_local_codecs"
        if not self.send_codecs:
            print "Not ready to send the codecs"
            return
        codecs = self.session.fssession.props.codecs
        if not codecs:
            print "Codecs are not ready"
            return
        if not self.send_codecs and \
                not self.session.fssession.codecs_need_resend(self.last_codecs,
                                                              codecs):
            print "Codecs have not changed"
            return
        self.last_codecs = codecs
        print "sending local codecs"
        for c in codecs:
            print "Sending local codec from %s %s" % \
                (self.id, c.to_string())
        self.connect.send_codecs(self.participant.id, self.id, codecs)

    def send_stream_codecs(self):
        if not self.connect.is_server:
            return
        if not self.session.fssession.props.codecs:
            return
        codecs = self.fsstream.props.negotiated_codecs
        if codecs:
            self.session.send_stream_codecs(codecs, self)

    def recv_codecs_changed(self, codecs):
        self.participant.recv_codecs_changed()


    def __remove_from_send_codecs_to(self, participant):
        self.send_codecs_to.remote(participant)


    def send_codecs_to(self, participant):
        codecs = self.fsstream.props.negotiated_codecs
        print "sending stream %s codecs from %s to %s" % \
              (self.id, self.participant.id, participant.id)
        if codecs:
            participant.connect.send_codecs(participant.id, self.id, codecs,
                                            self.participant.id)            


class FsUIParticipant (GObject.Object):
    "Wraps one FsParticipant, is one user remote contact"

    level = GObject.Property(type=float)
    
    def __init__(self, connect, id, pipeline, mainui):
        GObject.Object.__init__(self)
        self.connect = connect
        self.id = id
        self.pipeline = pipeline
        self.mainui = mainui
        self.fsparticipant = pipeline.conf.new_participant()
        self.outcv = threading.Condition()
        self.funnel = None
        self.make_widget()
        self.streams = {}
        if VIDEO:
            self.streams[int(Farstream.MediaType.VIDEO)] = \
                pipeline.videosession.new_stream(
                int(Farstream.MediaType.VIDEO), self)
        if AUDIO:
            self.streams[int(Farstream.MediaType.AUDIO)] = \
              pipeline.audiosession.new_stream(
                int(Farstream.MediaType.AUDIO), self)
        
    def candidate(self, media, candidate):
        "Callback for the network object."
        self.streams[media].candidate(candidate)
    def candidates_done(self, media):
        "Callback for the network object."
        self.streams[media].candidates_done()
    def codecs(self, media, codecs):
        "Callback for the network object."
        self.streams[media].codecs(codecs)
    def send_local_codecs(self):
        "Callback for the network object."
        for id in self.streams:
            self.streams[id].send_local_codecs()

    def make_widget(self):
        "Make the widget of the participant's video stream."
        Gdk.threads_enter()
        self.builder = Gtk.Builder()
        self.builder.add_from_file(builderprefix + "user-frame.ui")
        self.userframe = self.builder.get_object("user_frame")
        #self.builder.get_object("frame_label").set_text(self.cname)
        self.builder.connect_signals(self)
        self.label = Gtk.Label()
        self.label.set_alignment(0,0)
        self.label.show()
        self.mainui.hbox_add(self.userframe, self.label)
        volume_scale = self.builder.get_object("volume_scale")
        self.volume_adjustment = self.builder.get_object("volume_adjustment")
        volume_scale.props.adjustment = self.volume_adjustment
        self.mute_button = self.builder.get_object("mute_button")
        self.mute_button.bind_property("active", volume_scale, "sensitive",
                                       GObject.BindingFlags.SYNC_CREATE |
                                       GObject.BindingFlags.INVERT_BOOLEAN)

    def candidate(self, candidate):
        "Callback for the network object."
        self.candidates.append(candidate)
    def candidates_done(self):
        "Callback for the network object."
        if TRANSMITTER == "rawudp":
            self.fsstream.force_remote_candidates(self.candidates)
        else:
            self.fsstream.add_remote_candidates(self.candidates)
        self.candidates = []
    def codecs(self, codecs):
        "Callback for the network object. Set the codecs"

        print "Remote codecs"
        for c in codecs:
            print "Got remote codec from %s/%s %s" % \
                  (self.participant.id, self.id, c.to_string())
        oldcodecs = self.fsstream.props.remote_codecs
        if Farstream.codec_list_are_equal(oldcodecs,codecs):
            return
        try:
            self.fsstream.set_remote_codecs(codecs)
        except AttributeError:
            print "Tried to set codecs with 0 codec"
        self.send_local_codecs()
        self.send_stream_codecs()


    def send_local_codecs(self):
        "Callback for the network object."
        print "send_local_codecs"
        self.send_codecs = True
        self.check_send_local_codecs()

    def codecs_changed(self):
        print "codecs changed"
        self.check_send_local_codecs()
        self.send_stream_codecs()

    def check_send_local_codecs(self):
        "Internal function to send our local codecs when they're ready"
        print "check_send_local_codecs"
        if not self.send_codecs:
            print "Not ready to send the codecs"
            return
        codecs = self.session.fssession.props.codecs
        if not codecs:
            print "Codecs are not ready"
            return
        if not self.send_codecs and \
                not self.session.fssession.codecs_need_resend(self.last_codecs,
                                                              codecs):
            print "Codecs have not changed"
            return
        self.last_codecs = codecs
        print "sending local codecs"
        for c in codecs:
            print "Sending local codec from %s %s" % \
                (self.id, c.to_string())
        self.connect.send_codecs(self.participant.id, self.id, codecs)

    def send_stream_codecs(self):
        if not self.connect.is_server:
            return
        if not self.session.fssession.props.codecs:
            return
        codecs = self.fsstream.props.negotiated_codecs
        if codecs:
            self.session.send_stream_codecs(codecs, self)

    def recv_codecs_changed(self, codecs):
        self.participant.recv_codecs_changed()


    def __remove_from_send_codecs_to(self, participant):
        self.send_codecs_to.remote(participant)


    def send_codecs_to(self, participant):
        codecs = self.fsstream.props.negotiated_codecs
        print "sending stream %s codecs from %s to %s" % \
              (self.id, self.participant.id, participant.id)
        if codecs:
            participant.connect.send_codecs(participant.id, self.id, codecs,
                                            self.participant.id)            


class FsUIParticipant (GObject.Object):
    "Wraps one FsParticipant, is one user remote contact"

    level = GObject.Property(type=float)
    
    def __init__(self, connect, id, pipeline, mainui):
        GObject.Object.__init__(self)
        self.connect = connect
        self.id = id
        self.pipeline = pipeline
        self.mainui = mainui
        self.fsparticipant = pipeline.conf.new_participant()
        self.outcv = threading.Condition()
        self.funnel = None
        self.make_widget()
        self.streams = {}
        print "NEW PARTICIPANT: ", self
        if VIDEO:
            self.streams[int(Farstream.MediaType.VIDEO)] = \
                pipeline.videosession.new_stream(
                int(Farstream.MediaType.VIDEO), self)
        if AUDIO:
            self.streams[int(Farstream.MediaType.AUDIO)] = \
              pipeline.audiosession.new_stream(
                int(Farstream.MediaType.AUDIO), self)
        
    def candidate(self, media, candidate):
        "Callback for the network object."
        self.streams[media].candidate(candidate)
    def candidates_done(self, media):
        "Callback for the network object."
        self.streams[media].candidates_done()
    def codecs(self, media, codecs):
        "Callback for the network object."
        self.streams[media].codecs(codecs)
    def send_local_codecs(self):
        "Callback for the network object."
        for id in self.streams:
            self.streams[id].send_local_codecs()

    def make_widget(self):
        "Make the widget of the participant's video stream."
        Gdk.threads_enter()
        self.builder = Gtk.Builder()
        self.builder.add_from_file(builderprefix + "user-frame.ui")
        self.userframe = self.builder.get_object("user_frame")
        #self.builder.get_object("frame_label").set_text(self.cname)
        self.builder.connect_signals(self)
        self.label = Gtk.Label()
        self.label.set_alignment(0,0)
        self.label.show()
        self.mainui.hbox_add(self.userframe, self.label)
        volume_scale = self.builder.get_object("volume_scale")
        self.volume_adjustment = self.builder.get_object("volume_adjustment")
        volume_scale.props.adjustment = self.volume_adjustment
        self.mute_button = self.builder.get_object("mute_button")
        self.mute_button.bind_property("active", volume_scale, "sensitive",
                                       GObject.BindingFlags.SYNC_CREATE |
                                       GObject.BindingFlags.INVERT_BOOLEAN)
        self.bind_property("level", volume_scale, "fill-level", 0)
        Gdk.threads_leave()


    def realize(self, widget, *args):
        """From the realize signal, used to create the video sink
        The video sink will be created here, but will only be linked when the
        pad arrives and link_video_sink() is called.
        """

        if not VIDEO:
            return
        try:
            self.videosink.get_by_interface(GstVideo.VideoOverlay).expose()
        except AttributeError:
            try:
                self.outcv.acquire()
                self.xid = widget.get_window().get_xid()
                self.outcv.notifyAll()
            finally:
                self.outcv.release()
            

    def have_size(self, pad, info, nonedata):
        "Callback on the first buffer to know the drawingarea size"
        if (info.get_event().type != Gst.EventType.CAPS):
            return Gst.PadProbeReturn.OK
        
        caps = info.get_event().parse_caps()
        x = caps.get_structure(0).get_int("width")[1]
        y = caps.get_structure(0).get_int("height")[1]
        Gdk.threads_enter()
        self.builder.get_object("user_drawingarea").set_size_request(x,y)
        Gdk.threads_leave()
        return Gst.PadProbeReturn.OK

    def link_video_sink(self, pad):
        """Link the video sink

        Wait for the xid to appear, when it is known, create the video sink
        """

        print "LINK VIDEO SINK"
        try:
            self.outcv.acquire()
            while not hasattr(self, 'xid'):
                self.outcv.wait()
        finally:
            self.outcv.release()
        if self.xid is None:
            return

        if not hasattr(self, 'videosink'):
            self.videosink = make_video_sink(self.pipeline.pipeline, self.xid,
                                             "uservideosink")
            self.pipeline.pipeline.add(self.videosink)
            self.funnel = Gst.ElementFactory.make("funnel", None)
            self.pipeline.pipeline.add(self.funnel)
            self.funnel.link(self.videosink)
            self.havesize = self.videosink.get_static_pad("sink").add_probe(Gst.PadProbeType.EVENT_DOWNSTREAM, self.have_size, None)

            self.videosink.set_state(Gst.State.PLAYING)
            self.funnel.set_state(Gst.State.PLAYING)

        print >>sys.stderr, "LINKING VIDEO SINK"
        pad.link(self.funnel.get_request_pad("sink_%u"))

    def destroy(self):
        if VIDEO:
            try:
                self.videosink.get_static_pad("sink").disconnect_handler(self.havesize)
                pass
            except AttributeError:
                pass
            self.streams = {}
            self.builder.get_object("user_drawingarea").disconnect_by_func(self.realize)
            self.streams = {}
            self.outcv.acquire()
            del self.xid
            self.videosink.set_locked_state(True)
            self.funnel.set_locked_state(True)
            self.videosink.set_state(Gst.State.NULL)
            self.funnel.set_state(Gst.State.NULL)
            self.pipeline.pipeline.remove(self.videosink)
            self.pipeline.pipeline.remove(self.funnel)
            del self.videosink
            del self.funnel
            self.outcv.release()
        Gdk.threads_enter()
        self.userframe.destroy()
        self.label.destroy()
        Gdk.threads_leave()

    def error(self):
        "Callback for the network object."
        if self.id == 1:
            self.mainui.fatal_error("<b>Disconnected from server</b>")
        else:
            print "ERROR ON %d" % (self.id)

    def recv_codecs_changed(self):
        codecs = {}
        for s in self.streams:
            codec = self.streams[s].fsstream.props.current_recv_codecs
            mediatype = self.streams[s].session.fssession.get_property("media-type")
            if len(codec):
                if mediatype in codecs:
                    codecs[mediatype] += codec
                else:
                    codecs[mediatype] = codec
        str = ""
        for mt in codecs:
            str += "<big>" +mt.value_nick.title() + "</big>:\n"
            for c in codecs[mt]:
                str += "  <b>%s</b>: %s %s\n" % (c.id, 
                                                 c.encoding_name,
                                                 c.clock_rate)
        self.label.set_markup(str)

    def send_codecs_to(self, participant):
        for sid in self.streams:
            self.streams[sid].send_codecs_to(participant)
    

class FsMainUI:
    "The main UI and its different callbacks"
    
    def __init__(self, mode, ip, port):
        self.mode = mode
        self.pipeline = FsUIPipeline()
        self.pipeline.codecs_changed_audio = self.reset_audio_codecs
        self.pipeline.codecs_changed_video = self.reset_video_codecs
        self.builder = Gtk.Builder()
        self.builder.add_from_file(builderprefix + "main-window.ui")
        self.builder.connect_signals(self)
        self.mainwindow = self.builder.get_object("main_window")
        self.audio_combobox = self.builder.get_object("audio_combobox")
        self.video_combobox = self.builder.get_object("video_combobox")
        volume_scale =  self.builder.get_object("volume_scale")
        self.volume_adjustment = Gtk.Adjustment(lower=0, upper=100, value=100)
        volume_scale.props.adjustment = self.volume_adjustment
        mute_button = self.builder.get_object("mute_button")
        mute_button.bind_property("active", volume_scale, "sensitive",
                                  GObject.BindingFlags.SYNC_CREATE |
                                  GObject.BindingFlags.INVERT_BOOLEAN)
        self.pipeline.bind_property("level", volume_scale, "fill-level",
                                  GObject.BindingFlags.SYNC_CREATE)
        self.volume_adjustment.connect("value-changed", lambda adj, nonevalue:
                                           self.pipeline.set_volume(adj.props.value), None)
        liststore = Gtk.ListStore(str, object)
        self.audio_combobox.set_model(liststore)
        cell = Gtk.CellRendererText()
        self.audio_combobox.pack_start(cell, True)
        self.audio_combobox.add_attribute(cell, 'text', 0)
        self.reset_audio_codecs()
        liststore = Gtk.ListStore(str, object)
        self.video_combobox.set_model(liststore)
        cell = Gtk.CellRendererText()
        self.video_combobox.pack_start(cell, True)
        self.video_combobox.add_attribute(cell, 'text', 0)
        self.reset_video_codecs()

        if mode == CLIENT:
            self.client = FsUIClient(ip, port, FsUIParticipant,
                                     self.pipeline, self)
            self.builder.get_object("info_label").set_markup(
                "Connected to %s:%s" % (ip, port))
        elif mode == SERVER:
            self.server = FsUIListener(port, FsUIServer, FsUIParticipant,
                                       self.pipeline, self)
            self.builder.get_object("info_label").set_markup(
                "Expecting connections on port %s" %
                (self.server.port))

        
        self.mainwindow.show()

    def reset_codecs(self, combobox, fssession):
        liststore = combobox.get_model()
        current = fssession.get_property("current-send-codec")
        if liststore:
            liststore.clear()
        for c in fssession.props.codecs_without_config:
            str = ("%s: %s/%s %s" % (c.id, 
                                     c.media_type.value_nick,
                                     c.encoding_name,
                                     c.clock_rate))
            iter = liststore.append([str, c])
            if current and c and current.id == c.id:
                combobox.set_active_iter(iter)
                print "active: "+ c.to_string()

    def reset_audio_codecs(self):
        if AUDIO:
            self.reset_codecs(self.audio_combobox,
                              self.pipeline.audiosession.fssession)

    def reset_video_codecs(self):
        if VIDEO:
            self.reset_codecs(self.video_combobox,
                              self.pipeline.videosession.fssession)

    def combobox_changed_cb(self, combobox, fssession):
        liststore = combobox.get_model()
        iter = combobox.get_active_iter()
        if iter:
            codec = liststore.get_value(iter, 1)
            print codec.to_string()
            fssession.set_send_codec(codec)

    def audio_combobox_changed_cb(self, combobox):
        self.combobox_changed_cb(combobox, self.pipeline.audiosession.fssession)
    
    def video_combobox_changed_cb(self, combobox):
        self.combobox_changed_cb(combobox, self.pipeline.videosession.fssession)
        
    def toggled_mute(self, button):
        self.pipeline.set_mute(button.props.active)


    def realize(self, widget, *args):
        "Callback from the realize event of the widget to make the preview sink"
        if not VIDEO:
            return
        try:
            self.preview.get_by_interface(GstVideo.VideoOverlay).expose()
        except AttributeError:
            self.preview = self.pipeline.make_video_preview(widget.get_window().get_xid(),
                                                            self.newsize)

    def newsize (self, x, y):
        self.builder.get_object("preview_drawingarea").set_size_request(x,y)
        
    def shutdown(self, widget=None):
        Gtk.main_quit()
        
    def hbox_add(self, widget, label):
        table = self.builder.get_object("users_table")
        x = table.get_properties("n-columns")[0]
        table.attach(widget, x, x+1, 0, 1)
        table.attach(label, x, x+1, 1, 3, xpadding=6)

    def __del__(self):
        self.mainwindow.destroy()

    def fatal_error(self, errormsg):
        Gdk.threads_enter()
        dialog = Gtk.MessageDialog(self.mainwindow,
                                   Gtk.DialogFlags.MODAL,
                                   Gtk.MessageType.ERROR,
                                   Gtk.ButtonsType.OK)
        dialog.set_markup(errormsg);
        dialog.run()
        dialog.destroy()
        Gtk.main_quit()
        Gdk.threads_leave()

    def show_dtmf(self, button):
        try:
            self.dtmf_builder.present()
        except AttributeError:
            self.dtmf_builder = Gtk.Builder()
            self.dtmf_builder.add_from_file(builderprefix + "dtmf.ui")
            self.dtmf_builder.connect_signals(self)
            

    def dtmf_start(self, button):
        self.pipeline.audiosession.dtmf_start(button.get_label())
                                              
    def dtmf_stop(self, button):
        try:
            self.pipeline.audiosession.dtmf_stop()
        except AttributeError:
            pass
    def dtmf_destroy(self, button):
        self.dtmf_builder.get_object("dtmf_window").destroy()
        del self.dtmf_builder



class FsUIStartup:
    "Displays the startup window and then creates the FsMainUI"
    
    def __init__(self):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(builderprefix + "startup.ui")
        self.dialog = self.builder.get_object("neworconnect_dialog")
        self.builder.get_object("spinbutton_adjustment").set_value(9893)
        self.builder.connect_signals(self)
        self.dialog.show()
        self.acted = False

    def action(self, mode):
        port = int(self.builder.get_object("spinbutton_adjustment").get_value())
        ip = self.builder.get_object("newip_entry").get_text()
        try:
            self.ui = FsMainUI(mode, ip, port)
            self.acted = True
            self.dialog.destroy()
            del self.dialog
            del self.builder
        except socket.error, e:
            dialog = Gtk.MessageDialog(self.dialog,
                                       Gtk.DialogFlags.MODAL,
                                       Gtk.MessageType.ERROR,
                                       Gtk.ButtonsType.OK)
            dialog.set_markup("<b>Could not connect to %s %d</b>" % (ip,port))
            dialog.format_secondary_markup(e[1])
            dialog.run()
            dialog.destroy()
        
    def new_server(self, widget):
        self.action(SERVER)

    def connect(self, widget):
        self.action(CLIENT)
        

    def quit(self, widget):
        if not self.acted:
            Gtk.main_quit()




if __name__ == "__main__":
    if len(sys.argv) >= 2:
        CAMERA = sys.argv[1]
    else:
        CAMERA = None
    
    GLib.threads_init()
    Gdk.threads_init()
    Gst.init(sys.argv)
    startup = FsUIStartup()
    Gtk.main()
    del startup
    gc.collect()
