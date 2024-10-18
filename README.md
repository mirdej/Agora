# Agora Library

Facilitate ESP-Now communication setup

The Agora is the public space where ESP32 devices meet and communicate. ESPs gather in tribes, each tribe has a unique **tribename** and each tribe has its **Guru**

- Each tribe can have several members.
- Each tribe has exactly one Guru.
- Each device can be member or guru of several tribes.


## Model

When a device is powered on it goes out looking for the tribes it belongs to. If it is a Guru it will just listen for lost membvers wanting to join. If it is a trime member then it will start shouting out into the Agora trying to find its Guru.

1) If a guru hears a lost member looking for the tribe it will send an (private) invitation to join the tribe.
2) The new device will answer with its name and MAC address.
3) The guru will then communicate its own name and MAC address. 
4) The device is now officialy a follower of the guru and member of the tribe.

A guru will keep a list of all tribe members and will periodically check if each follower is still listening.
A member who does not reply in a timely manner to the guru will be kicked out of the tribe.

Members will pay attention if they hear from their gurus from time to time. If not, they feel abandoned and will start the search for the tribe again.


## Implementation

We work with fixed size messages for the rendez-vous protocol (padded with dots '.'), this lets us determine quickly if a message is potentially part of the process