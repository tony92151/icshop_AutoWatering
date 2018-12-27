#define EncoderA_Pin 4 //(2)
#define EncoderB_Pin 5 //(3)
#define EncoderSW_Pin 6 //(4)

void init_encoder(){
    
}


char Encoder_Read () { //(int?)
  bool O_A, N_A, N_B;
  O_A = N_A;
  N_A = digitalRead(EncoderA_Pin);
  N_B = digitalRead(EncoderB_Pin);
  if (O_A && !N_A) { //(O_A==1 && N_A==0)
    if (N_B) { //N_B==1
      return 1;
    }
    else {
      return -1;
    }
  }
  if (!O_A && N_A) { //(O_A==0 && N_A==1)
    if (!N_B) { //N_B==0
      return 1;
    }
    else {
      return -1;
    }
  }
  return 0;
}