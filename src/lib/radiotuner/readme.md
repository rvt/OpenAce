

## Class RadioTuner {}

* Module that handles timing's of protocols and configures the Radio's accordingly to listen and request for sending on the radio frequencies according to the protocols and required maximum power output
* Module keeps track on what protocols traffic is received on and will prioritise the timeslots based on received data.  
* Can configure and controle 1..N Radio modules. Usually 1 or 2 will be installed


### Limitations

* All radio's must beable to handle all protocols. This module won't beable to decide if a spacific radio is not beable to handle given modulations or frequencies.

#### General operation




## Class CountryRegulation {}

Class that holds the Zone regulations

Per zone the following information is stored:

 * Frequency
 * Timings when data can be received
 * Delay after CAD (when data was received)
 * Maximum power output in the zone
 * Maximum and minimum time between transmissions

 The class can also decide the current `CountryRegulations::Zone`` to be used  (private method) and what Regulation should be used for a specific protocol.

