
unsigned char not_(unsigned char num, int pos=0){
  return num ^ (1<<pos);
}

unsigned char neg(unsigned char num){
  num &= 0x0f;
  num = not_(num, 3);
  num = not_(num, 2);
  num = not_(num, 1);
  num = not_(num, 0);
  return num;
}