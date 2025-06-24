# IOCP & Redis Cluster를 활용한 MMO 게임 서버 프로젝트

<br>

## [소개]

### ㅇMMO Server
 
  #### 1. 성능 최적화 (Performance Optimization)
    - Circular Buffer를 활용하여 메모리 사용 효율화
    - 객체 풀(Object Pool) 설계를 통한 동적 메모리 할당 최소화
    - 대량 데이터 송수신 시 char*기반 처리로 오버헤드 최소화
    - atomic, boost::lockfree_queue, tbb::concurrent_hash_map 등을 활용하여 mutex 사용 최소화

  #### 2. 보안 & 인증 (Security & Authentication)
    - 게임 시작 시 Login Server에서 JWT 토큰 발급 → Center Server에 인증 요청
    - 서버 간 이동 시 JWT 토큰 기반 인증 시스템으로 유저 식별 및 보안 강화
    - SQL Injection 방지를 위해 Prepared Statement 적용

  #### 3. 서버 구조 및 확장성 설계 (Scalable Multi-Server Architecture)
    - Center Server를 중심으로 Login, Matching, Raid, Channel Server 등 역할 분리
    - 기능별 서버 간 통신 구조를 통해 확장성, 유지보수성, 부하 분산 확보
    - Redis Cluster를 각 서버에서 공용 캐시로 사용하여, 서버 간 데이터 일관성 및 빠른 응답성 확보
    - 모든 서버에 고유 번호를 부여하여, 서버 간 통신의 일관성과 향후 서버 추가에 대비한 유연한 구조 설계
    
  #### 4. 게임 시스템 로직 (Game System Logic)
    - 인벤토리 시스템: 아이템 획득, 삭제, 슬롯 이동, 장비 강화
    - 장비 강화 시스템: 난수 엔진 기반 확률 적용
    - 경험치 획득 및 레벨업 시스템 구현
    - 레벨 그룹별 레이드 매칭 시스템 구현 
    - 실시간 레이드 전투 시스템 구현 (제한 시간 초과 또는 몬스터 HP 0 시 종료)
    - 주요 데이터(레이드 점수, 장비 강화)는 Redis Cluster와 MySQL에 실시간 이중 동기화 

<br> 

### ㅇCenter Server - User Connection Management & Server Migration with JWT Issuance
   - 전체 유저 접속 관리 및 인증 처리
   - JWT 발급을 통한 서버 간 이동 및 보안 강화
   - 채널 서버별 유저 수 상태 관리 및 전달
   - 중요 데이터 Redis와 MySQL에 실시간 이중 동기화
   - 로그아웃시, Redis → MySQL 일괄 데이터 저장 처리
<br> 

### ㅇLogin Server - User Authentication & Session Initialization
   - 유저 로그인 요청 시, MySQL → Redis Cluster로 유저 데이터 로드  
   - JWT 토큰을 생성하여 Redis에 저장하고 유저에게 전달
   - 인증 및 초기화 완료 후 유저와 연결을 종료
<br> 

### ㅇChannel Server - Monster Hunting, Inventory Management, and Equipment Enhancement System
   - 서버 내 채널 이동 관리
   - 몬스터 사냥을 통한 경험치 획득 및 레벨업 처리
   - 인벤토리 아이템 획득/삭제/이동 처리
   - 장비 강화 기능 처리
   - 레이드 랭킹 조회

<br> 

### ㅇMatching Server - User Group Matching & Room Assignment for Game Initialization
   - 레벨 그룹 단위 유저 매칭
   - 매칭 성공 시, 생성 가능한 방 번호와 매칭에 사용된 유저들의 PK를 게임 서버에 전달 
   - 레이드 종료 후, 게임 서버로부터 종료된 방 번호를 수신하여 재등록

<br> 

### ㅇGame Server - Room Data Generation & User Synchronization for Gameplay
   - 매칭 서버로부터 방 번호 및 유저 PK 수신 후, 방 생성   
   - 모든 매칭 유저가 접속하면, 확률 기반으로 랜덤 맵을 생성하고 몬스터 HP 및 제한 시간 설정 후 유저에게 정보 전송 
   - 전투 중 틱레이트 기반의 실시간 게임 상태 동기화
   - 몬스터 HP가 0이 되거나 제한 시간이 초과되면, 조건에 따라 다른 방식으로 레이드 종료 처리

<br> 


#### ㅇ 프로젝트 소개서 - [MMOServer_Project_Intro.pdf](https://github.com/user-attachments/files/20421605/MMOServer_Project_Intro.pdf)





<br>  

## [Flow Chart]


- #### User Connect
![Game Server Connect drawio](https://github.com/user-attachments/assets/efa77c95-aed4-487f-9d56-eb2d21af1d27)

<br>

- #### User Logout or Disconnect
![logout drawio](https://github.com/user-attachments/assets/3428ac25-9ae1-4800-acb5-5b5cbc9dd620)

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
<br>

## [Test Output]
- #### User Connect & Logout & Syncronization
![User Connect   Logout   Syncronization](https://github.com/user-attachments/assets/97c207a0-864a-4b1a-8206-8164c79321bc)


1. 유저가 게임 시작을 누르면, 로그인 정보를 Login Server로 전송합니다.
   
2. Login Server는 유저의 게임 시작 요청을 수신하면, MySQL에서 유저 정보와 인벤토리 데이터를 가져와 Redis Cluster에 로드합니다.

3. 모든 데이터를 Redis Cluster에 정상적으로 로드한 후, JWT 토큰을 생성합니다.

4. 생성된 JWT 토큰을 Redis Cluster에 저장하고, 동시에 유저에게 전송합니다.

5. 유저는 전달받은 JWT 토큰으로 중앙 서버에 접속 요청을 전송합니다.

6. 중앙 서버는 Redis Cluster에 저장된 JWT 토큰과 유저가 전송한 토큰을 비교하여, 일치하는 경우 접속을 허가합니다.

7. 유저가 로그아웃하면, 중앙 서버는 Redis Cluster에 저장된 유저 데이터를 MySQL과 동기화합니다.

<br> 

- #### Server & Channel Transition
![Server   Channel Transition](https://github.com/user-attachments/assets/66656260-fbaf-419b-b0e0-f1f23bda2f88)



1. 유저가 중앙 서버에 연결되면, 서버 선택 페이지로 이동합니다.

2. 서버를 선택하면 중앙 서버로부터 해당 서버의 주소와 JWT 토큰을 받아, 선택한 서버로 연결을 시도합니다.

3. 서버와의 연결이 성공하면, 채널 이동 페이지로 전환됩니다.

4. 유저가 서버 선택 페이지로 돌아갈 때, 접속 중인 서버는 중앙 서버에 유저 퇴장 신호를 보내어 서버 유저 수를 감소시킵니다.
 
5. 유저는 중앙 서버의 유저 수 감소 처리가 완료될 수 있도록 1초 동안 대기합니다.

6. 채널 및 서버 이동 중에도 유저 수를 지속적으로 동기화합니다.

<br>

- #### Raid Start & Raid End (Mob Hp 0)
![Raid Start   Raid End (Mob Hp 0)](https://github.com/user-attachments/assets/e2c99091-fabb-4900-b39a-fbed30f885d6)

1. 레이드가 시작되면, 유저는 서버로부터 참여한 다른 유저들의 정보를 전달받습니다.

2. 전투가 시작되면, 서버는 레이드 상태 동기화를 위해 틱레이트 기반으로 동기화 메시지를 주기적으로 전송합니다.

3. 레이드 몬스터의 HP가 0이 되면 레이드가 종료됩니다.

4. 레이드 종료 후, 유저는 자신의 점수와 다른 유저들의 점수를 확인할 수 있습니다.

5. 서버는 각 유저의 획득 점수가 기존 최고 점수보다 높을 경우, 해당 점수로 랭킹을 업데이트합니다.

<br>

- #### Raid Start & Raid End (Time Out)
![Raid Start   Raid End (Time Out)](https://github.com/user-attachments/assets/30675120-415f-4f0a-97ec-84087febf25a)


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



