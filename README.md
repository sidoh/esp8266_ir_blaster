# esp8266_ir_blaster
Little REST server to send IR commands to an esp8266-based circuit

## Questions

### What chip?

I used a NodeMCU v1. This'll probably work just fine with other chips that have 2+ GPIO pins.

### What's it do?

This is just a little RESTful service that accepts `POST /ir` to send infrared signals and `GET /ir` to dump IR signals it notices.

### Aren't there already, like, infinity of these?

Yeah, I guess.

### Why bother?

Mostly to learn about the Arduino SDK and the ESP8266.

I was also interested in securing it a little more than the ones I ran across. Mostly because I felt adventurous. I wish my life were interesting enough to require protection from leet hackers turning on my fan.

### Circuit?

Pretty simple. GPIO wasn't good enough to drive the LED by itself, so I used an NPN transistor. Needed a 100Ω resistor on Vin for the 38 KHz IR receiver to filter out noise.

![Breadboard](http://i.imgur.com/tZ1t2Kd.png)

Obviously my actual breadboard was more of a disaster:

![Actual breadboard](http://imgur.com/T2L18OA.jpg)

My prototype board was slightly less disastrous, but it's still pretty ugly:

![Prototype front](http://imgur.com/t3sOlxJ.jpg)
![Prototype back](http://imgur.com/J58NKaC.jpg)

I put two LEDs on the finished product, as you can see. #fancy

## Routes

There are a couple of admin-y things. Defaut username and password is admin/esp8266:

#### `GET /`

You can open up `/` to configure some security settings in the form of a JSON blob (I'm lazy). Settings are persisted as a file in flash memory. The admin username and password are only used for the admin-y routes.

#### `POST /settings`

Accepts a JSON blob of settings. Need to auth with the username/password.

#### `POST /update`

Accepts a firmware binary for OTA updates.

The actual service routes follow. If you specify an HMAC secret, you'll need to include some security headers in the request (detailed below in the "security" section).

#### `GET /ir`

Does a long poll while it waits for an IR signal to show up. When one does, dumps some plaintext info about the seen IR signal.

#### `POST /ir`

Accepts a JSON blob specifying an IR command and blasts it to the IR LEDs. Currently I only support `raw` and `sony` as types because I'm lazy. 

Examples:

```json
{"type":"raw","data":[750,1800,750,1800,2000,550,2000,550,750,1800,750,1800,750,1800,2000,550,750,1800,2000,550,2000,550,750,1800,750,1800,750,1800,2000,550,750,1800,750,1800,2000,550,2000,550,750,1800,750,1800,750,1800,2000,550,750],"pwm_frequency":38}
```

(This turns on/off my Honeywell fan)

```json
{"type":"sony","data":"2704","num_bits":12}
```

(This turns on/off my TV)

## Security

If you don't care about this just leave settings as-is and none of this will apply.

If you specify an HMAC secret, requests sent to `/ir` have to include a (within 20 seconds of current) timestamp and an HMAC or the server will respond with a 412 error.

The HMAC should sign the concatenation of:

* The path (should always be `/ir` unless we add other routes).
* The request body (raw JSON blob).
* The included timestamp.
