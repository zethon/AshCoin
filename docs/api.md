# AshCoin API & Services

This document provides detailed information for:

* All of the HTTP endpoints
* All of the HTTP REST services implemented 
* All of the WebSocket RPCs

## HTTP Endpoints

HTTP endpoints are the user interface for the local instance of AshCoin. These endpoints allow users to interact with the service. 

#### `/index`

This is the main entry point for the application. T

#### `/createTx`

This page provides a means to create a transaction in the current node. 

## REST Services

REST services are under the `/rest` path and accept JSON payloads as POST HTTP calls. All REST endpoints are prepended with `/rest`.

#### `/rest/createTx`

Creates a transaction on the current node with the given parameters:

* `privatekey`
* `destination`
* `amount`

An example request may look like: 

```json
{
    "privatekey" : "1b3f78b45456dcfc3a2421da1d9961abd944b7e8a7c2ccc809a7ea92e200eeb1h",
    "destination" : "1LahaosvBaCG4EbDamyvuRmcrqc5P2iv7t",
    "amount: 5.25
}
```

A successful response will look like: 

```json
{
    "success" : true
}
```

If there is an error it will be in the `error` field:
```json
{
    "success" : false,
    "error": "insufficient funds"
}
```

## WebSocket RPC

The Websocket RPC is primarily used for node-to-node communication. The communication protocol is JSON based. The procedure name and the procedure type are at a minimum required in every call.

The RPC is generally a request-response type of communication, however there are some instances where the *response* must invoke some type of *request*. Because of this the JSON payload must self-identify as a *request* or a *response*. 

For example consider this `summary` request

```json
{
    "message":"summary",
    "message-type":"request"
}
```

And this response:

```json
{
    "blocks": 
    [
        {
            "data": "Gensis Block created 2021-01-09 07:51:25 EST",
            "difficulty": 1,
            "hash": "e41ad9e82072e708d4e58512dbc7e7dad72daf1dfbf9ab5c547fbd82f5a49824",
            ...
        },
        {
            "data": "coinbase block#49",
            "difficulty": 3,
            "hash": "000f1937b4b2b266578c2ade3f4bb416f763da5bb6f7f74ae1e0e0625495fd14",
            ...
        }
    ],
    "cumdiff": 162,
    "message": "summary",
    "message-type": "response"
}
```

#### `summary`

The summary command returns basic information about the current node's copy of the chain such as the genesis block, the latest blockl and the cummulative difficulty.
