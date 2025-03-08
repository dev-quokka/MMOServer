# IOCP & Redis Cluster를 활용한 MMO 게임 서버 프로젝트

<br> 

## [소개]

### ㅇGame Server (Redis Cluster) 

  #### 1. 네트워크 최적화
    - Circular Buffer를 활용하여 메모리 사용 최적화
    - 동적 할당 최소화를 위한 설계 적용
    - 대량 데이터 송수신 시 vector 대신 char*형 활용으로 성능 최적화
    - atomic, boost::lockfree_queue, tbb::concurrent_hash_map 등을 활용하여 mutex 사용 최소화
    - Redis 통신 시 try-catch 문을 적용으로 예외 처리 강화
    - MySQL 동기화가 필요한 데이터는 Session Server를 통해 처리하여 부하 분산

  #### 2. 인벤토리 (장비, 소비, 재료)
    - 아이템 획득, 삭제, 슬롯 이동, 장비 강화 시스템 구현
    - 난수 생성 엔진을 활용한 장비 강화 성공 확률 차등 적용

  #### 3. 레이드
    - 레벨 그룹별 레이드 매칭 시스템 구현 
    - 실시간 레이드 전투 시스템 구현 (제한 시간 초과 또는 몬스터 HP 0시 종료) 
    - UDP 기반 IOCP 통신을 활용한 실시간 몬스터 HP 동기화
    - 레이드 랭킹 시스템 구현

  #### 4. 유저 시스템
    - Session Server에서 생성된 JWT 토큰 검증을 통해 접속 요청 유저를 이중 확인하여 보안성 강화
    - 경험치 증가, 레벨업 알고리즘 구현
    - 레벨별 요구 경험치량 설정
    
<br> 

### ㅇSession Server (MySQL, Redis Cluster) - User Authentication & Connection Game Server For Syncronization
   - JWT 토큰을 활용한 유저 인증 보안 강화
   - 유저의 게임 시작 요청시, MySQL에서 유저 정보 및 인벤토리 데이터를 Redis Cluster로 load 
   - 유저 로그아웃시, Redis Cluster에 업데이트된 데이터를 MySQL에 동기화 (Batch Update)

<br> 

### ㅇClient
   - 게임 시작시 Session Server에서 JWT 토큰을 발급 받아 Game Server에 인증 요청
   - 보안 강화를 위해 클라이언트에서 연산 처리를 하지 않도록 설계


<br>  

## [Flow Chart]

- #### User Connect
![Game Server Connect](https://github.com/user-attachments/assets/95b759f4-6a82-4131-9753-174e3fb480ee)

<br>

- #### Raid Start
![Raid Start drawio](https://github.com/user-attachments/assets/c6b74c45-9f12-4ffe-bfbb-c6615d92d8e0)


<br>

- #### Raid End (Time Out)
![Raid Time End](https://github.com/user-attachments/assets/f6fdd216-52fe-40bd-b2b4-600e57a04169)


<br>

- #### Raid End (Mob HP 0)
![Raid mob dead drawio](https://github.com/user-attachments/assets/75b87074-1368-4c1e-9e1f-5430e699937f)


<br>

- #### User Logout or Disconnect
![User Logout drawio](https://github.com/user-attachments/assets/805f11d2-250a-4d60-8874-fad43366fc27)

<br>
<br>

## [Test Output]
- #### User Connect & Logout & Syncronization
![접속, 접속종료](https://github.com/user-attachments/assets/e9d78268-0fb4-40b1-970f-538dd39c6fc3)

1. 게임 서버가 시작되면 세션 서버와 연결합니다.
2. 세션 서버로 유저가 게임 시작을 요청하면, MySQL에서 유저 정보와 인벤토리 데이터를 가져와서 Redis Cluster에 load합니다.
3. 모든 데이터를 Redis Cluster에 정상적으로 load 한 후, JWT 토큰을 생성합니다.
4. 생성된 JWT 토큰을 Redis Cluster에 저장하고, 유저에게도 전송합니다.
5. 유저는 받은 JWT 토큰을 포함하여 게임 서버에 접속 요청을 전송합니다.
6. 게임 서버는 Redis Cluster에서 저장된 JWT토큰과 유저가 전송한 토큰과 비교하여 일치하면 접속을 허가합니다.
7. 유저가 로그아웃하면, 게임 서버는 세션 서버에 유저 PK와 함께 로그아웃 신호를 보냅니다.
8. 세션 서버는 받은 PK를 기반으로 Redis Cluster에 저장된 데이터를 MySQL과 동기화합니다.

<br>

- #### Raid Start & Raid End & Ranking Update (Mob Hp 0)
![레이드 몹 잡고 랭킹 확인](https://github.com/user-attachments/assets/94eafd7f-08e5-416b-9731-b4465a948b1d)

1. 레이드 몬스터의 HP가 0이 되면 레이드가 종료되며, 내 점수와 다른 유저의 점수를 반환받아 확인할 수 있습니다.
2. 레이드 몬스터의 HP가 0이 되는 순간 추가적인 데미지가 들어오면, 남은 몹 HP만큼만 점수를 획득할 수 있습니다.
3. 획득한 점수를 기존 최고 점수와 비교하여 더 높을 경우 랭킹을 업데이트 합니다.

<br>

- #### Raid Start & Raid End (Time Out)
![타임아웃되면 0점마무리](https://github.com/user-attachments/assets/92dce42d-1204-4fd6-9ccc-69ecd7b07bfb)

1. 제한시간이 종료되면 레이드가 자동으로 종료됩니다.
2. 레이드 중 획득한 점수만 서버에 기록되며, 레이드 종료시 내 점수와 다른 유저의 점수를 반환받아 확인할 수 있습니다.

<br>

- #### Raid Group Check by Level
![레이드 그룹 체크](https://github.com/user-attachments/assets/f74b7422-cac0-431a-b95a-740e1b5d1dd4)

- 다른 레벨의 그룹이면 매칭이 불가능합니다.

<br>

- #### Equipment Enhancement
![장비강화](https://github.com/user-attachments/assets/3dc8088e-f5b7-47d5-bef0-d6fe364b13a1)

- 현재 강화 수치에 따라 확률을 차등 적용하여, 강화 성공 또는 실패를 반환합니다.



