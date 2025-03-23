# SokuAI
A Soku AI that (hopefully) will beat everyone's ass lol

# AITrainer
## Protocol
All packets are formed of a single byte opcode followed by opcode specific data.
All numbers are stored in little endian (least significant byte first).
The game (Client) connects to the remote (Server).
```
0        1      N
-----------------
| Opcode | Data |
-----------------
```

## Trainer protocol
### HELLO (0x00) Client/Server
Data size: 4 bytes

- Bytes 0 - 3: Magic number (Always 0xF56E9D2A)

This packet should be sent to the client by the server when it connects.
No other command should be processed if this packet hasn't been sent.
The remote should check if the magic number is correct before acknowledging this request.
An hello patch with the magic number should be sent back if the handshake is successful.

Possible error:
- INVALID_MAGIC: The magic number is incorrect

### GOODBYE (0x01) Client/Server
Data size: 0 byte

Sent when either one of the parties want to end the connection with the peer.

### SPEED (0x02) Server only
Data size: 4 bytes

- Bytes 0 - 3: Ticks per second

Changes the game speed to the desired ticks per second. The normal speed of the game is 60 ticks per second.
If this packet is not sent, the game will run at 60 tps.

### DISPLAY (0x03) Server only
Data size: 1 byte

- Byte 0: Enabled (0 or 1)

Whether to not draw the game or not. It will be enabled by default. This is useful to save some processing power.

### SOUND (0x04) Server only
Data size: 2 bytes

Byte 0: SFX volume (0-100)
Byte 1: Music volume (0-100)

Changes the volume of the game. Defaults to the values set in the game configuration.

### START_VS_PLAYER_GAME (0x05) Server only
### START_VS_COM_GAME (0x17) Server only
Data size: 52 bytes

- Byte 0: Stage ID (0 - 19)
- Byte 1: Music ID (0 - 19)
- Bytes 2-26: Left character data
- Bytes 27-51: Right character data

Character data:
- 4 bytes: Character ID (0 - 19) ([id list](https://github.com/SokuDev/SokuLib/blob/d93bcbf25da12bbd9cc41737658cfc78060e44d9/src/Data/Character.hpp#L16))
- 20 bytes: Deck
- 1 byte: Palette (0 - 7)

Sent to the Client to start a new game.

Possible errors:
- INVALID_LEFT_DECK: The specified left player's deck is not valid
- INVALID_LEFT_CHARACTER: The specified left player's character id is not valid
- INVALID_LEFT_PALETTE: The specified left player's palette is not valid
- INVALID_RIGHT_DECK: The specified right player's deck is not valid
- INVALID_RIGHT_CHARACTER: The specified right player's character id is not valid
- INVALID_RIGHT_PALETTE: The specified right player's palette is not valid
- INVALID_MUSIC: The specified music is not valid
- INVALID_STAGE: The specified stage is not valid
- STILL_PLAYING: A game is still being played or the game is not ready to start a new game

### FAULT (0x06) Client only
Data size: 4 bytes

- Bytes 0 - 3: Faulting address
- Bytes 4 - 7: Exception code

The client crashed and cannot continue execution.

### ERROR (0x07) Client only
Data size: 1 byte

- Byte 0: Error ID

And error occurred and the request could not be processed.

Error codes:
#### INVALID_OPCODE (0x00)
The opcode specified doesn't exist or is not supported

#### INVALID_PACKET (0x01)
The packet specified doesn't have a proper size

#### INVALID_LEFT_DECK (0x02)
The specified left player's deck is not valid

#### INVALID_LEFT_CHARACTER (0x03)
The specified left player's character id is not valid

#### INVALID_LEFT_PALETTE (0x04)
The specified left player's palette is not valid

#### INVALID_RIGHT_DECK (0x05)
The specified right player's deck is not valid

#### INVALID_RIGHT_CHARACTER (0x06)
The specified right player's character id is not valid

#### INVALID_RIGHT_PALETTE (0x07)
The specified right player's palette is not valid

#### INVALID_MUSIC (0x08)
The specified music is not valid

#### INVALID_STAGE (0x09)
The specified stage is not valid

#### INVALID_MAGIC (0x0A)
The magic number is incorrect

#### HELLO_NOT_SENT (0x0B)
The hello packet hasn't yet been successfully processed

#### STILL_PLAYING (0x0C)
A game is still being played or the game is not ready to start a new game

### GAME_FRAME (0x08) Client only
Data size: 215 + nbObjects * 17

- Bytes 0 - 104: Left character state;
- Bytes 105 - 209: Right character state;
- Byte 210: Displayed weather (List [here](https://github.com/SokuDev/SokuLib/blob/51486a0c400201313f6afff1155e8f84bbb9d809/src/Core/Weather.hpp#L11);
- Byte 211: Active weather (List [here](https://github.com/SokuDev/SokuLib/blob/51486a0c400201313f6afff1155e8f84bbb9d809/src/Core/Weather.hpp#L11));
- Bytes 212 - 213: Weather timer (0 - 999);
- Bytes 214 - N: Other objects

Character state:
- 1 byte: Direction (-1 if facing left or 1 if facing right)
- 8 bytes: (float, float) Position
- 2 bytes: Soku action (List [here](https://github.com/SokuDev/SokuLib/blob/51486a0c400201313f6afff1155e8f84bbb9d809/src/Core/CharacterManager.hpp#L25)));
- 2 bytes: Sequence index (Block index in the action);
- 2 bytes: Pose index (Animation index);
- 2 bytes: Pose subframe (Number of frames this pose has been shown);
- 4 bytes: Frame count (Number of frames this sequence has been shown);
- 2 bytes: Combo damage;
- 2 bytes: Combo limit;
- 1 byte: Airborne (Whether the character is in the air or not);
- 2 bytes: Time stop timer;
- 2 bytes: Hit stop timer;
- 2 bytes: Hp left;
- 1 byte: Air dash count (Number of air movement used);
- 2 bytes: Spirit left;
- 2 bytes: Max spirit;
- 2 bytes: Untech;
- 2 bytes: Healing charm time left;
- 2 bytes: Confusion debuff time left;
- 2 bytes: Sword of rapture debuff time left;
- 1 byte: Score (0 - 2);
- 10 bytes: (byte, byte)\[5\] Hand. First byte is card ID and second is cost, repeated 5 times (Card ID + Cost 0xFF is no card);
- 2 bytes: Card gauge;
- 15 bytes: All skills level (0xFF means not learned);
- 1 byte: Fan level (0 - 4 -> Number of Tengu fan(s) used);
- 1 byte: Rods level (0 - 4 -> Number of Control rod(s) used);
- 1 byte: Books level (0 - 4 -> Number of Grimoire(s) used);
- 1 byte: Doll level (0 - 4 -> Number of Sacrificial Doll(s) used);
- 1 byte: Drop level (0 - 3 -> Number of Heavenly drop(s) used);
- 2 bytes: Drop invulnerability time left;
- 2 bytes: Super armor hp used;
- 2 bytes: Image index;
- 20 bytes: Character specific data;
- 1 byte: Child objects count

Object:
- 1 byte: Direction;
- 8 bytes: (float, float) Position
- 2 bytes: Action index;
- 2 bytes: Sequence index;
- 2 bytes: Hit stop;
- 2 bytes: Image (sprite) index;

To know the number of objects, look at the child objects field for both states.
The left character's objects are placed first, followed by the right character's.

Note that the mod makes no effort to hide information (It will send the actual cards in mountain vapor, and send the actual weather in Aurora).
It's up to the server to post process the frames to hide the information it needs.

All times are in frames.

Sent every game tick. Expected response: GAME_INPUTS

### GAME_INPUTS (0x09) Server only
Data size: 4 bytes

Bytes 0 - 1: Left inputs
Bytes 2 - 3: Right inputs

Inputs:
- Bit 0: A key
- Bit 1: B key
- Bit 2: C key
- Bit 3: D key
- Bit 4: Use spellcard
- Bit 5: Switch card
- Bits 6 - 7: Horizontal axis (-1 left, 0 neutral or 1 right)
- Bits 8 - 9: Vertical axis (-1 up, 0 neutral or 1 down)
- Bits 10 - 15: Ignored

### GAME_CANCEL (0x0A) Server only
Data size: 0 byte

Requests the on going game to be ended prematurely.
If no game is in progress, it has no effect.

Note that this is only a request. The client can decide to postpone or ignore the request entirely.
The server should not expect the game to end as soon as this opcode is sent and keep sending GAME_INPUT
packets until a GAME_ENDED is received for example.

### GAME_ENDED (0x0B) Client only
Data size: 3 byte

Byte 0: Winner side (0 if the game was canceled, 1 if left won and 2 if the right won)
Byte 1: Left player's score (number of round won)
Byte 2: Right player's score (number of round won)

Sent at the end of any game to the server

### OK (0x0C) Client only
Data size: 0 byte

Sent after every request that succeeded and doesn't return a specific opcode.