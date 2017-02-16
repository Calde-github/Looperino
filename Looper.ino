    #include <math.h>

    /*
     * THANKS TO
     * @ http://www.freestompboxes.org/viewtopic.php?f=1&t=26184
     * to the user pz to have starting all
     * to the user smithoid to initial analysis
     * to the user ashimoke for sharing his data and support
     * @ http://fuzzysynth.blogspot.it/2015/06/digitech-jam-man.html
     * to fuzzy music for sharing the work on protocol
     * @ http://chemiker1981.blogspot.it/2010/10/1-reading-midi-clock-to-read-midi-clock.html
     * to chemiker1981 for midi clock code
     *
     * r0 - initial release for testing
     * r1 - LoopTime = effective lenght
    */

    // MIDI STATUS BYTES
    byte midi_start    = 0xfa;
    byte midi_stop     = 0xfc;
    byte midi_clock    = 0xf8;
    byte midi_continue = 0xfb;

    // INTERNALS
    byte play_flag = 0;
    byte incoming_data;

    // DATA FOR CALCULATIONS
    unsigned int QPM = 4;  // QUARTERS PER MEASURE
    unsigned int NOM = 1;  // MINIMUM NUMBERS OF MEASURE OF THE LOOP
    unsigned int PPQ = 24; // TICK OR PULSES PER QUARTER
    unsigned int BPM = 0;  // BEATS PER MINUTE

    // TIMINGS AND COUNTERS
    unsigned int Tick_Counter = 0;
    unsigned long jamsync_measure_timer;
    unsigned long jamsync_loop_timer;
    unsigned long jamsync_link_timer ;


    // SYSEX ARRAYS
    unsigned int jamsync_sync[] {0x00,0xF0,0x00,0x00,0x10,0x7F,0x62,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x02,0x04,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0xF7};
    unsigned int jamsync_link[] {0x00,0xF0,0x00,0x00,0x10,0x7F,0x62,0x01,0x00,0x01,0x01,0xF7};
    unsigned int jamsync_stop[] {0x00,0xF0,0x00,0x00,0x10,0x7F,0x62,0x01,0x4A,0x01,0x04,0x46,0x30,0x42,0x02,0x04,0x40,0x03,0x4E,0x46,0x2E,0x40,0x04,0x5C,0xF7};

    void setup() {
      Serial.begin(31250);
       jamsync_link_timer = micros();
       }

    void loop() {
     
      // READ THE INCOMING MIDI
       CheckSerial();
     
      // KEEP LINK ACTIVE
       if (micros() - jamsync_link_timer > 400000) {sendLink();}
     
      // SEND SYNC COMMAND
       if (Tick_Counter == NOM*QPM*PPQ) {
        GetBPM();
         Sendjamsync_sync();
        Tick_Counter = 0;
       }
       
    }
       
       void CheckSerial() {
          if(Serial.available() > 0) {
             incoming_data = Serial.read();
         
             if(incoming_data == midi_start) {
                play_flag = 1;
            if (BPM > 0) {Sendjamsync_sync();}
                jamsync_measure_timer = micros();
                Tick_Counter = 0;
             }
             else if(incoming_data == midi_continue) {
                play_flag = 1;
             }
             else if(incoming_data == midi_stop) {
                play_flag = 0;
            sendStop();
             }
             else if((incoming_data == midi_clock) && (play_flag == 1)) {
                Tick_Counter++;
             }
          }
       }
     
      void sendLink() {
        for (int i = 1; i < 12; i++) { Serial.write(jamsync_link[i]); }
        jamsync_link_timer = micros();
      }
     
      void sendStop() {
        for (int i = 1; i < 25; i++) { Serial.write(jamsync_stop[i]); }
      }
       
       void GetBPM() {
          jamsync_measure_timer = micros() - jamsync_measure_timer;
        jamsync_loop_timer = jamsync_measure_timer;
          BPM = round(QPM * 60000000.0 / jamsync_measure_timer);
          jamsync_measure_timer = micros();
       }
       
       void Sendjamsync_sync(){
         unsigned long LoopTime;
        int b163 = 0;
        int w;
          int x;
          int y;
          int z=0; //xor checksum

       
        // LoopTime = floor(1000* NOM * QPM * 60.0 / BPM);
        LoopTime = floor(jamsync_loop_timer / 1000.0);
        x = floor(log(LoopTime/2000.0)/log(4.0));
        b163 = (LoopTime/(2000.0 * pow(4.0,x)))>2;
        y = 2 * pow(2,b163) * pow(4,x);
        w = floor(LoopTime / y);

          // BPM
          jamsync_sync[8]  = 66 + 8 * ((63<BPM) && (BPM<128) || BPM>191) ;
          jamsync_sync[12] = (4*BPM>127 && 4*BPM<256)*(4*BPM-128) +
                             (2*BPM>127 && 2*BPM<256)*(2*BPM -128) +
                             (BPM>127 && BPM<256)*(BPM-128);
          jamsync_sync[13] = 1*(BPM>127)+66;
          
          // lenght
          jamsync_sync[16] = 64 + 8*b163;
          jamsync_sync[21] = 64 + x;
          jamsync_sync[20] = 128 *(0.001*w-1);
          jamsync_sync[19] = pow(128.0,2) *(0.001*w-1 - jamsync_sync[20]/128.0);
          jamsync_sync[18] = pow(128.0,3) *(0.001*w-1 - jamsync_sync[20]/128.0 - jamsync_sync[19]/pow(128.0,2));
          
          // command
          jamsync_sync[22] = 5;
          
          // checksum
          for (int i = 8; i < 23; i++) {
             z = z ^ int(jamsync_sync[i]); // checksum XOR
          }
          jamsync_sync[23] = z;
          
          for (int i = 1; i < 25; i++) {
             Serial.write(int(jamsync_sync[i]));
          }
       }
       
