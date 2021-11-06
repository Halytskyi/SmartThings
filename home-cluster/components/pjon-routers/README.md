# PJON routers

Routers use for make possible communication between different devices with different [PJON](https://github.com/gioblu/PJON/tree/13.0/src/strategies) strategies via [PJON-gRPC](https://github.com/Halytskyi/PJON-gRPC).

- 2 x [Serial-SoftwareBitBang_RxTx_busA](Serial-SoftwareBitBang_RxTx_busA) - routers for communication between RPi (through Serial) and remote devices (through SoftwareBitBang) by transmit-receive messages
- 2 x [Serial-SoftwareBitBang_Tx_busB](Serial-SoftwareBitBang_Tx_busB) - routers for receiving messages from remote devices (connected via SoftwareBitBang) to RPi (through Serial)

For High Availability, one pair of [Serial-SoftwareBitBang_RxTx_busA](Serial-SoftwareBitBang_RxTx_busA) and [Serial-SoftwareBitBang_Tx_busB](Serial-SoftwareBitBang_Tx_busB) connected to Master01, another one - to Master02

Compatible with [PJON v13.0](https://github.com/gioblu/PJON/tree/13.0) and [PJON-gRPC v4.0](https://github.com/Halytskyi/PJON-gRPC/tree/4.0)

## Device Photos

[<img src="images/pjon-routers_1.jpeg" width="250"/>](images/pjon-routers_1.jpeg)
[<img src="images/pjon-routers_2.jpeg" width="255"/>](images/pjon-routers_2.jpeg)
[<img src="images/pjon-routers_3.jpeg" width="312"/>](images/pjon-routers_3.jpeg)
[<img src="images/pjon-routers_4.jpeg" width="327"/>](images/pjon-routers_4.jpeg)
[<img src="images/pjon-routers_5.jpeg" width="215"/>](images/pjon-routers_5.jpeg)
[<img src="images/pjon-routers_6.jpeg" width="217"/>](images/pjon-routers_6.jpeg)
[<img src="images/pjon-routers_7.jpeg" width="315"/>](images/pjon-routers_7.jpeg)
