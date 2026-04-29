# 프로젝트 컨텍스트: ESP32-IDF 기반 시스템 개발

이 파일은 AI 에이전트가 본 프로젝트의 구조, 스타일 가이드 및 기술적 의사결정 사항을 이해하기 위한 가이드라인입니다.

## 🛠 기술 스택
- **Framework:** ESP-IDF v5.4.4
- **Language:** C (C99/C11 표준 지향)
- **Build System:** CMake / idf.py. 루트의 env.bat를 먼저 실행하여 IDF 환경 설정을 반영해야 함 
- **Target Chip:** ESP32-기본

## 📂 프로젝트 구조
- `main/`: 메인 로직 및 `app_main.c` 위치
- `components/`: 재사용 가능한 사용자 정의 컴포넌트
- `managed_components/`: IDF Component Manager를 통해 추가된 라이브러리
- `sdkconfig`: ESP-IDF 구성 설정 파일

## 📝 코딩 스타일 및 규칙
1. **Naming Convention:**
    - 함수/변수: `snake_case` (예: `sensor_read_task`)
    - 컴포넌트 접두사: 함수명 앞에 컴포넌트 이름을 붙임 (예: `wifi_init_sta()`)
    - 상수/매크로: `UPPER_SNAKE_CASE`
2. **Error Handling:**
    - 모든 ESP-IDF API 호출 시 `esp_err_t` 리턴값을 확인하고 `ESP_ERROR_CHECK()` 또는 적절한 로그 처리를 수행할 것.
3. **Logging:**
    - `printf` 대신 `ESP_LOGI`, `ESP_LOGE`, `ESP_LOGW` 매크로를 사용할 것. (Tag는 파일 상단에 정의)
4. **FreeRTOS 활용:**
    - Task 생성 시 Stack Size와 Priority를 명시적으로 관리할 것.
    - 공유 자원 접근 시 `Mutex` 또는 `Semaphore` 필수 사용.

## 🔗 참조할 기존 코드 패턴 (Reference)
- **NVS 저장:** `components/storage/nvs_helper.c`의 패턴 사용
- **메모리 관리:** 정적 할당보다는 `pvPortMalloc` 또는 `heap_caps_malloc` 사용 선호

## ⚙️ 하드웨어 제어 규칙
- **DC 모터 구동:** 메인 트랙 제어는 정밀한 제어를 위해 `MCPWM`을 사용함. 단, ESP32의 MCPWM Operator 자원(총 6개) 부족 시 터렛/포 마운트와 같은 액세서리용 모터는 `GPIO`를 이용한 ON/OFF 방식으로 제어할 수 있음.
- **서보 모터:** **`LEDC`**를 사용하여 제어할 것.

## ⚠️ 주의 사항
- `sdkconfig`를 직접 수정하지 말고, 설정 변경이 필요하면 `menuconfig` 항목을 언급해줄 것.
- IDF 5.4.4 버전의 API를 사용할 것. 다른 버전은 고려하지 않고 있음
- **하드웨어 핀(Pin) 맵 및 상세 기능 정의는 프로젝트 루트의 `README.md` 파일을 최우선으로 참조할 것.**
