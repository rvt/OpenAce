GDL90 Encode/Decode for Aviation Devices and Software

This is a single header file to help encode/decode standard GDL90 messages.

Note: I always compile with the -std=c++17 option. You may encounter
problems if you use a much early C++ standard.

This file does NOT send or receive messages or perform any network discovery.
That is left to your application.

The GDL90 spec can be found here: https://www.faa.gov/nextgen/programs/adsb/Archival/media/GDL90_Public_ICD_RevA.PDF.
This header file currently implements only what is in section 3, plus Foreflight extensions.
Sections 4, 5, and 6 will be filled in based on demand (feel free to help).

The ForeFlight GDL90 extensions are discussed here: https://www.foreflight.com/connect/spec/.
They are fully supported by this header file. There are currently two ForeFlight-specific messages.

After instantiating this class, it is highly recommended that you call
the self_test() method and check that the return value is true.
The self_test() method is also a good place to look for examples of usage.

Typically one encodes a message using its encode() method, 
then packs() it to prepare it for sending. The packing adds escape sequences,
CRC, and start/end delimeters. The packed message is typically sent by the caller
as a UDP datagram.

Upon receipt (e.g., of a UDP datagram), one unpacks() the message, then calls 
id_decode() to determine which MESSAGE_ID has been received, then calls the 
appropriate decode() method.

Some message fields are complicated, so there are additional encode/decode
methods dedicated to converting to/from "normal" values into the encodings
expected by GDL90.

All methods return a bool indicating whether the inputs were legal and
the method was fully performed. If the inputs are not legal, it does not currently
tell you why, but you can set a breakpoint at GDL90::error() and see where it died.
You can also set the abort_on_error flag to have it die immediately.

This is all open source. See the LICENSE.md (MIT) file for details.

Bob Alfieri
<br>
Chapel Hill, NC
