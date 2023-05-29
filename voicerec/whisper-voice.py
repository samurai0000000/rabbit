#!/usr/bin/python

import os
import paho.mqtt.client as mqtt
import whisper

def on_connect(client, userdata, flags, rc):
    print('Connected to MQTT broker')
    client.subscribe('rabbit/voice/wav')

def on_message(client, userdata, msg):
    wavfile = msg.payload.decode('ascii')
    print('transcribe: ' + wavfile)
    try:
        audio = whisper.load_audio(wavfile)
        audio = whisper.pad_or_trim(audio)
        mel = whisper.log_mel_spectrogram(audio).to(model.device)
        options = whisper.DecodingOptions(fp16 = False, language = 'en')
        result = whisper.decode(model, mel, options)
        print(result.text)
        client.publish('rabbit/voice/transcribed', result.text)
    except Exception as e:
        print('Error: ' + str(e))
    os.remove(wavfile)

#
# Program starts
#

model = whisper.load_model("small.en")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect('rabbit', 1883, 60)
try:
    client.loop_forever()
except KeyboardInterrupt:
    print('bye!')
