# Settings

## Overview

The default settings file is name `.ash_config` and is located in the users home directoy.

If this file does not exist when the program is read, then it will be automatically genated in the user's home folder. These locations are typicalling in the following folders on each OS:

* MacOS - `/Users/<username>`
* Ubuntu - `/home/<username>`
* Windows - `C:\Users\<username>`

## Custom Config

The default configuration file can be override using the `--config` or `-c` option on the command line.

```bash
$> ash -c /path/to/my/config.file
```

## Settings

All settings are required to be in the configuration file with valid values. An invalid configuration file will cause an error and the program will not run. 

#### `chain.reset.enable`
If you join a mining network and the remote network has a different Genesis Block, setting this to true will erase your block database and download the remote blockhain (i.e. *passive mode*). 

#### `database.folder`
The folder in which to persist the local copy of the blockchain.

#### `logs.file.enabled`

Whether or not log messages should be saved to a file.

#### `logs.file.folder`

The folder in which to store log messages if they are being saved to a file.

#### `logs.level`

The level of log messages.

#### `mining.autostart`

Whether or not mining should start automatically when the service is started.

#### `peers.file`

The file from which to load the list of peers.

#### `rest.autoload`

Whether or not load a browser with the REST interface when the process is started in console mode.

#### `rest.port`

The port on which the HTTP service should listen.

#### `websocket.port`

The websocket port on which the websocket service should listen.
