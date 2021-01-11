# TETRIS Generator

You will find a pre-compiled executables for the ESPL TETRIS block generator in this directory:

General documentation on the opponents can be found here: https://github.com/alxhoff/FreeRTOS-Emulator/wiki/Opponents

## Example Usage:

```
./tetris_generator [-v] [--host HOSTNAME] [--port PORT]
```

## Parameters:

- `--verbose`, `-v`
Shows detailed logging output if toggled on
*Default:* false

- `--host`, `-h`
Hostname of the system running the Game/Emulator
*Default:* localhost

- `--port`, `-p`
Port the transmission (TX) socket. (RX will use port+1)
*Default:* 1234

## Message Sequence Charts

You can find some figures below to get an idea of the available commands and responses.

*Hint:* The dotted lines are initiated by the Generator, you only have to implement the Emulator-side of the communication by sending the commands `MODE,SEED,LIST,RESET,NEXT` and receiving the corresponding result.

Set values by appending an `=VALUE` to the command. If successful the response will look like this: `[COMMAND] OK`. In the case of an error, the following should be returned: `[COMMAND] FAIL`.

![LIST Command](./svg/tetris_generator_list.svg)
![MODE Command](./svg/tetris_generator_mode.svg)
![SEED Command](./svg/tetris_generator_seed.svg)
![NEXT/RESET Command](./svg/tetris_generator_next.svg)
