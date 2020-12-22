# README

## Introduction

The aim of this project is to demonstrate that it is possible to create an independent voting client for the Estonian i-voting system. We decided to build a proof-of-concept voting client on top of a microcontroller to show that the voter does not have to rely on the official closed source voting application. However, the design of the proof-of-concept voting client should also reduce the risks posed by malware and telemetry. By relying on a single purpose microcontroller, it is possible to significantly reduce the trust base. This makes it more difficult to deploy malware that could interfere with the voting process. In addition, as the microcontroller runs only the voting application, third party software can not violate vote secrecy by recoding which choice was made by the voter. Thus, in practice the microcontroller based voting client could have multiple advantages compared to an official PC based voting application.

The proof-of-concept implementation only supports Estonian mobile-ID for authenticating the voter and for digitally signing the ballot. ID-card support is not implemented for the proof-of-concept device.

The voting client is built to be compatible with the implementation of the IVXV protocol, which is used by the Estonian i-voting system. The server-side source code of IVXV along with its documentation can be found from https://github.com/vvk-ehk/ivxv. 
