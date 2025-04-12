# IOCP & Redis Cluster를 활용한 MMO 게임 서버 프로젝트

<br> 

## [소개]

### ㅇMMO Server
 
  #### 1. 네트워크 최적화
    - Circular Buffer를 활용하여 메모리 사용 최적화
    - 동적 할당 최소화를 위한 객체 풀 설계 적용
    - 대량 데이터 송수신 시 vector 대신 char*형 활용으로 성능 최적화
    - atomic, boost::lockfree_queue, tbb::concurrent_hash_map 등을 활용하여 mutex 사용 최소화
    - Redis 통신 시 try-catch 문 적용으로 예외 처리 강화
    - MySQL 동기화가 필요한 데이터는 Gateway Server를 통해 처리하여 부하 분산

  #### 2. 인벤토리 (장비, 소비, 재료)
    - 아이템 획득, 삭제, 슬롯 이동, 장비 강화 시스템 구현
    - 난수 생성 엔진을 활용한 장비 강화 성공 확률 차등 적용

  #### 3. 레이드
    - 레벨 그룹별 레이드 매칭 시스템 구현 
    - 실시간 레이드 전투 시스템 구현 (제한 시간 초과 또는 몬스터 HP 0시 종료) 
    - TCP와 UDP를 활용한 하이브리드 IOCP 통신을 통해 몬스터 HP 및 전투 정보 실시간 동기화
    - 레이드 랭킹 시스템 구현

  #### 4. 유저 시스템
    - Gateway Server에서 생성된 JWT 토큰 검증을 통해 접속 요청 유저를 이중 확인하여 보안성 강화
    - 경험치 증가, 레벨업 알고리즘 구현

<br> 

### ㅇCenter Server - User Connection Management & Server Migration with JWT Issuance
   - 유저 게임 접속 관리 및 인증 처리
   - 채널 서버별 유저 수 상태 관리 및 전달
   - 레이드 매칭 요청 수신 및 매칭 완료 후 게임 서버 주소 전달
   - 레이드 랭킹 조회
   - 서버 이동 시 JWT 토큰 생성 및 발급을 통한 보안 강화

<br> 

### ㅇChannel Server - Monster Hunting, Inventory Management, and Equipment Enhancement System
   - 서버 내 채널 이동 관리
   - 몬스터 사냥을 통한 경험치 획득 및 레벨업
   - 인벤토리 내 아이템 획득, 삭제, 이동 관리
   - 장비 강화 시스템 제공

<br> 

### ㅇGateway Server - User Authentication & Connection Game Server For Syncronization
   - 유저 게임 시작 시 MySQL → Redis Cluster 데이터 로드
   - 유저 로그아웃 시 Redis Cluster → MySQL 데이터 일괄 동기화 (Batch Update)
   - JWT 토큰을 활용한 유저 인증 보안 강화

<br> 

### ㅇㅇMatching Server - User Group Matching & Room Assignment for Game Initialization
   - 레벨 그룹 단위 매칭 수행
   - 매칭 성공 시 중앙 서버 및 게임 서버로 방 번호 전달
   - 레이드 종료 후 방 번호 재등록

<br> 

### ㅇGame Server - Room Data Generation & User Synchronization for Gameplay
   - 매칭 서버로부터 방 번호 및 유저 정보 수신 후 게임 방 생성 (Mob HP, Timeout Duration)
   - 틱 레이트 기반 게임 상태 실시간 동기화
   - 레이드 진행 상태 및 종료 조건 관리 (Mob HP 0 or Time Out)
     
<br> 

### ㅇClient
   - 게임 시작시 Gateway Server에서 JWT 토큰을 발급 받아 Game Server에 인증 요청
   - 보안을 강화하기 위해 클라이언트의 연산 처리를 최소화하는 설계를 적용

<br> 

#### ㅇ기술스택 - C++, IOCP, MySQL, Redis Cluster, Docker, Boost, TBB, JWT


#### ㅇ 프로젝트 소개서 - [IOCP & Redis Cluster를 활용한 MMO 게임 서버 프로젝트.pdf](https://github.com/user-attachments/files/19143541/IOCP.Redis.Cluster.MMO.pdf)


<br>  

## [Flow Chart]
- #### MMO Server
![MMO SERVER Flow Chart 2 drawio](https://github.com/user-attachments/assets/49bf5da6-f458-4844-97d4-891410e1dcd6)


<br>

- #### User Connect
![Game Server Connect drawio](https://github.com/user-attachments/assets/0f726d5d-8282-4270-b0d4-b0ed3d378e5b)

<br>

- #### Move Server
![Move Channel drawio](https://github.com/user-attachments/assets/ab0ea345-f916-4371-a4da-82483e784ca8)


<br>

- #### Raid Matching
![Raid Start drawio](https://github.com/user-attachments/assets/f767bc79-8db3-4c6e-820b-d6b9a1d119b7)


<br>

- #### Raid Start
![Raid Game Start drawio](https://github.com/user-attachments/assets/6a371c20-4ef5-4c1b-beaf-baa65cbcfd7c)


<br>

- #### Raid End (Time Out)
![Raid Time End](https://github.com/user-attachments/assets/f6fdd216-52fe-40bd-b2b4-600e57a04169)


<br>

- #### Raid End (Mob HP 0)
![Raid mob dead drawio](https://github.com/user-attachments/assets/20b7c6db-6fc4-4d8e-a8cb-eabe75cf01da)



<br>

- #### User Logout or Disconnect
![logout drawio](https://github.com/user-attachments/assets/4c122802-c7cc-44c6-8a26-91cdc4d72342)


<br>
<br>

## [Test Output]
- #### Server & Channel Transition
![Server   Channel Transition](https://github.com/user-attachments/assets/1665ab64-d757-43b5-8782-6910498a028b)


1. 유저가 중앙 서버에 연결되면, 서버 선택 페이지로 이동합니다.

2. 서버를 선택하면 중앙 서버로부터 해당 서버의 주소와 JWT 토큰을 받아, 선택한 서버로 연결을 시도합니다.

3. 서버와의 연결이 성공하면, 채널 이동 페이지로 전환됩니다.

4. 유저가 서버 선택 페이지로 돌아갈 때, 접속 중인 서버는 중앙 서버에 유저 퇴장 신호를 보내어 서버 유저 수를 감소시킵니다.
 
5. 유저는 중앙 서버의 유저 수 감소 처리가 완료될 수 있도록 1초 동안 대기합니다.

6. 채널 및 서버 이동 중에도 유저 수를 지속적으로 동기화합니다.

<br> 

- #### User Connect & Logout & Syncronization
![User Login   Logout](https://github.com/user-attachments/assets/2504378e-88c6-4b3f-a2bc-20cb1ab4981f)


1. 유저가 게임 시작을 누르면, 로그인 정보를 Gateway Server로 전송합니다.
2. Gateway Server는 유저의 게임 시작 요청을 수신하면, MySQL에서 유저 정보와 인벤토리 데이터를 가져와 Redis Cluster에 로드합니다.
3. 모든 데이터를 Redis Cluster에 정상적으로 로드한 후, JWT 토큰을 생성합니다.
4. 생성된 JWT 토큰을 Redis Cluster에 저장하고, 동시에 유저에게 전송합니다.
5. 유저는 전달받은 JWT 토큰으로 중앙 서버에 접속 요청을 전송합니다.
6. 중앙 서버는 Redis Cluster에 저장된 JWT 토큰과 유저가 전송한 토큰을 비교하여, 일치하는 경우 접속을 허가합니다.
7. 유저가 로그아웃하면, 중앙 서버는 Gateway Server에 유저 PK와 함께 로그아웃 신호를 전송합니다.
8. Gateway Server는 수신한 유저 PK를 기반으로, Redis Cluster에 저장된 유저 데이터를 MySQL과 동기화합니다.

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
![레이드 그룹별](https://github.com/user-attachments/assets/803de230-8b49-434c-9746-e3d4f310c9f9)


- 다른 레벨의 그룹이면 매칭이 불가능합니다.

<br>

- #### Equipment Enhancement
![장비강화](https://github.com/user-attachments/assets/3dc8088e-f5b7-47d5-bef0-d6fe364b13a1)

- 현재 강화 수치에 따라 확률을 차등 적용하여, 강화 성공 또는 실패를 반환합니다.



