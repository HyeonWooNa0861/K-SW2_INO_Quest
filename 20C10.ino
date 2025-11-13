/*
===========================================================
      IR Distance Sensor Curve-Fitting Automation
===========================================================
조건 충족 기능:
✔ Curve fitting 방식(P1,P2,P3) 시리얼로 입력받음
✔ 위치는 0,5,10,...,30cm 로 자동 증가
✔ 사용자는 Enter 키만 입력 → IR 측정 자동 수행
✔ 측정 완료 후 자동으로 curve-fitting 수행
✔ Arduino에서 바로 실행 가능한 보정 식 출력
===========================================================
*/

#define PIN_IR A0

// -----------------------------------------
#define MAX_DATA 20
float dist[MAX_DATA];     // 거리(mm)
float adc[MAX_DATA];      // ADC 값
int count = 0;

int poly_order = 0;       // 1,2,3차 선택
int next_position_mm = 0; // 다음 측정할 거리 (0→300까지)

// -----------------------------------------
int compareFloat(const void *a, const void *b) {
  float diff = (*(float*)a - *(float*)b);
  return (diff > 0) - (diff < 0);
}

// -----------------------------------------
// ADC 값 10회 읽어서 median 리턴
// -----------------------------------------
unsigned int readMedianADC() {
  const int N = 15;
  unsigned int buf[N];

  for (int i = 0; i < N; i++)
    buf[i] = analogRead(PIN_IR);

  qsort(buf, N, sizeof(unsigned int), compareFloat);
  return buf[N/2];
}

// ===========================================================
//   1차식 곡선 맞춤 (Linear)
// ===========================================================
void solveP1() {
  float Sx=0, Sy=0, Sxx=0, Sxy=0;
  int n = count;

  for(int i=0;i<n;i++){
    float x=adc[i], y=dist[i];
    Sx+=x; Sy+=y;
    Sxx+=x*x;
    Sxy+=x*y;
  }

  float denom = n*Sxx - Sx*Sx;
  float b = (n*Sxy - Sx*Sy)/denom;
  float a = (Sy - b*Sx)/n;

  Serial.println("\n=== Result: 1st-order Polynomial ===");
  Serial.print("DIST = ");
  Serial.print(a,6); Serial.print(" + ");
  Serial.print(b,6); Serial.println("*x;");
}

// ===========================================================
//   2차식 곡선 맞춤 (Quadratic)
// ===========================================================
void solveP2() {
  float Sx=0,Sx2=0,Sx3=0,Sx4=0;
  float Sy=0,Sxy=0,Sx2y=0;
  int n=count;

  for(int i=0;i<n;i++){
    float x=adc[i], y=dist[i], x2=x*x;
    Sx+=x; Sy+=y;
    Sx2+=x2;
    Sx3+=x2*x;
    Sx4+=x2*x2;
    Sxy+=x*y;
    Sx2y+=x2*y;
  }

  float A[3][4] = {
    { (float)n, Sx,  Sx2, Sy   },
    { Sx,       Sx2, Sx3, Sxy  },
    { Sx2,      Sx3, Sx4, Sx2y }
  };

  // 가우스 소거
  for(int i=0;i<3;i++){
    float p=A[i][i];
    for(int j=i;j<4;j++) A[i][j]/=p;

    for(int k=0;k<3;k++){
      if(k==i) continue;
      float m=A[k][i];
      for(int j=i;j<4;j++) A[k][j]-=m*A[i][j];
    }
  }

  float a=A[0][3], b=A[1][3], c=A[2][3];

  Serial.println("\n=== Result: 2nd-order Polynomial ===");
  Serial.print("DIST = ");
  Serial.print(a,6); Serial.print(" + ");
  Serial.print(b,6); Serial.print("*x + ");
  Serial.print(c,8); Serial.println("*x*x;");
}

// ===========================================================
//   3차식 곡선 맞춤 (Cubic)
// ===========================================================
void solveP3() {
  float Sx=0,Sx2=0,Sx3=0,Sx4=0,Sx5=0,Sx6=0;
  float Sy=0,Sxy=0,Sx2y=0,Sx3y=0;
  int n=count;

  for(int i=0;i<n;i++){
    float x=adc[i], y=dist[i];
    float x2=x*x, x3=x2*x;

    Sx+=x; Sy+=y;
    Sx2+=x2; Sx3+=x3;
    Sx4+=x2*x2;
    Sx5+=x3*x2;
    Sx6+=x3*x3;
    Sxy+=x*y;
    Sx2y+=x2*y;
    Sx3y+=x3*y;
  }

  float A[4][5] = {
    { (float)n, Sx,  Sx2, Sx3, Sy   },
    { Sx,       Sx2, Sx3, Sx4, Sxy  },
    { Sx2,      Sx3, Sx4, Sx5, Sx2y },
    { Sx3,      Sx4, Sx5, Sx6, Sx3y }
  };

  for(int i=0;i<4;i++){
    float p=A[i][i];
    for(int j=i;j<5;j++) A[i][j]/=p;

    for(int k=0;k<4;k++){
      if(k==i) continue;
      float m=A[k][i];
      for(int j=i;j<5;j++) A[k][j]-=m*A[i][j];
    }
  }

  float a=A[0][4], b=A[1][4], c=A[2][4], d=A[3][4];

  Serial.println("\n=== Result: 3rd-order Polynomial ===");
  Serial.print("equation = ");
  Serial.print(a,6); Serial.print(" + ");
  Serial.print(b,6); Serial.print("*x + ");
  Serial.print(c,8); Serial.print("*x*x + ");
  Serial.print(d,10); Serial.println("*x*x*x;");
}

// ===========================================================
// Setup
// ===========================================================
void setup() {
  Serial.begin(1000000);
  Serial.println("=== IR Sensor Curve Fitting ===");
  Serial.println("Enter P1, P2, P3 to choose polynomial order.");
}

// ===========================================================
// Loop
// ===========================================================
void loop() {

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // ------------ Polynomial 차수 설정 --------------
    if (cmd == "P1") { poly_order=1; next_position_mm=0; count=0; Serial.println("Order = 1 (Linear)"); return; }
    if (cmd == "P2") { poly_order=2; next_position_mm=0; count=0; Serial.println("Order = 2 (Quadratic)"); return; }
    if (cmd == "P3") { poly_order=3; next_position_mm=0; count=0; Serial.println("Order = 3 (Cubic)"); return; }

    // ------------ Enter 입력 → 측정 수행 -------------
    if (poly_order >= 1) {

      if (next_position_mm > 300) {

        Serial.println("\n=== All Data Collected ===");
        Serial.println("Performing Polynomial Curve-Fitting...\n");

        if (poly_order == 1) solveP1();
        if (poly_order == 2) solveP2();
        if (poly_order == 3) solveP3();

        Serial.println("\nDone.");
        poly_order = 0;
        return;
      }

      // 실제 측정 실행
      unsigned int median_adc = readMedianADC();

      dist[count] = next_position_mm;
      adc[count]  = median_adc;
      count++;

      Serial.print("Position ");
      Serial.print(next_position_mm);
      Serial.print(" mm → ADC=");
      Serial.println(median_adc);

      next_position_mm += 50;    // 0,50,100,...300
    }
  }
}
