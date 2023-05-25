#ifndef ServoManager_h // 2重インクルードを防ぐ
#define ServoManager_h

class ServoManager {
    public:
        ServoManager(); // ←コンストラクタと合わせる
        bool servo_home;
        void (ServoManager::*Servo_run)(void *args);
        void Servo_setup();
    
    private:
        void _Servo_run(void *args);
  
}; // ←セミコロン忘れずに
#endif
