BEWARE: AI-Generated Summary. To be used for **CASUAL FAMILIARIZATION ONLY**

### Rule #1: Transmit Power

You are limited in how "loudly" you can shout. The absolute limit is **1 Watt (30 dBm)** of effective isotropic radiated power (EIRP).

*   **What this means:** Your SX1262 module's output power (e.g., +14 dBm or +22 dBm) plus the gain from your antenna must not exceed 30 dBm.
*   **For your testing:** Most LoRa modules with typical small antennas will be well under this limit. For example, a module outputting +20 dBm with a 2 dBi antenna results in an EIRP of 22 dBm, which is perfectly legal. You don't need to worry about exceeding this unless you are attaching a large, high-gain antenna.

### Rule #2: Time on Air (Dwell Time) - *This is the most important one for you*

This is the rule you asked about specifically. To prevent a single device from hogging a frequency, the FCC has a "dwell time" limit.

*   **The Rule for "Digital Transmission Systems" (which includes LoRa):** If your signal's 6dB bandwidth is less than 250 kHz (which it will be for standard LoRa bandwidths like 125 kHz), the rule is: **you shall not occupy any single frequency for more than 400 milliseconds (0.4 seconds) in any 20-second period.**

*   **What this means for you:**
    1.  **Calculate your packet's Time on Air (ToA).** The driver provides the `sx126x_get_lora_time_on_air_in_ms()` function for this.
    2.  Let's say your packet takes **150 ms** to send.
    3.  After that transmission finishes, you **must not** transmit again *on that exact same frequency* for at least the next 20 seconds.
    4.  The easiest way to comply during testing is to simply add a long delay between transmissions in your sender's main loop (e.g., `sleep_ms(20000);`).

### Rule #3: Frequency Hopping - The "Solution" to Dwell Time

You might think the 400ms rule is very restrictive. This is where frequency hopping comes in. The FCC rules are different and much more relaxed if your system is a "Frequency Hopping Spread Spectrum" (FHSS) system.

*   **The Hopping Rule:** If your system hops between at least **25 different frequencies** (for bandwidths < 250 kHz), and it doesn't dwell on any single frequency for more than 400ms, the "20-second period" rule is effectively waived. You just need to make sure you use all the channels equally over time.

*   **What this means for you:**
    *   For initial testing, you can ignore hopping and just use a single frequency with a long delay to comply with the 400ms dwell time rule. This is the simplest approach.
    *   For a more advanced and robust final capstone project, implementing a simple frequency hopping scheme is an excellent goal. You would create a list of, say, 30 different frequencies within the 915 MHz band and have your transmitter cycle through them for each packet. This makes your system more robust against interference and a better "citizen" on the airwaves.