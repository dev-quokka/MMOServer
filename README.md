# IOCP & Redis Cluster를 활용한 MMO 게임 서버 프로젝트

<br>

## [소개]

### ㅇMMO Server
 
  #### 1. 성능 최적화 (Performance Optimization)
    - Circular Buffer를 활용하여 메모리 사용 효율화
    - 객체 풀(Object Pool) 설계를 통한 동적 메모리 할당 최소화
    - 대량 데이터 송수신 시 char*기반 처리로 오버헤드 최소화
    - atomic, boost::lockfree_queue, tbb::concurrent_hash_map 등을 활용하여 mutex 사용 최소화
    - MySQL 커넥션 풀 적용으로 커넥션 재사용 및 동시 요청 처리 성능 향상 
    - Redis Pipeline 및 Lua 스크립트(EVALSHA) 활용으로 다중 명령 처리 시 RTT 및 네트워크 비용 최소화

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
    - 상점 시스템: 로그인시 상점 아이템 목록 전달, 유저 보유 재화에 따른 아이템 구매 처리 및 획득
    - 패스 시스템: 로그인시 각 패스 정보 전달, 패스 미션을 통한 경험치 획득·레벨업 및 레벨별 보상 지급
    - 경험치 획득 및 레벨업 시스템 구현
    - 레벨 그룹별 레이드 매칭 시스템 구현 
    - 실시간 레이드 전투 시스템 구현 (제한 시간 초과 또는 몬스터 HP 0 시 종료)
    - 주요 데이터(레이드 점수, 장비 강화)는 Redis Cluster와 MySQL에 실시간 이중 동기화 
    

#### ㅇ 프로젝트 소개서 - [MMOServer_Project_Intro.pdf](https://github.com/user-attachments/files/28784576/_._._MMOGameServer.pdf)





<br>  

## [Flow Chart]


- #### User Connect
<img src="https://github.com/user-attachments/assets/efa77c95-aed4-487f-9d56-eb2d21af1d27" width="80%" alt="Game Server Connect drawio">

<br>

- #### Move Server
<img src="https://github.com/user-attachments/assets/ab0ea345-f916-4371-a4da-82483e784ca8" width="80%" alt="Move Channel drawio">

<br>

- #### Raid Matching
<img src="https://github.com/user-attachments/assets/f767bc79-8db3-4c6e-820b-d6b9a1d119b7" width="80%" alt="Raid Start drawio">

<br>

<br>

## [Test Output]

- #### User Connect & Logout & Syncronization
<img src="https://github.com/user-attachments/assets/97c207a0-864a-4b1a-8206-8164c79321bc" width="70%" alt="User Connect   Logout   Syncronization">

1. 유저가 게임 시작을 누르면, 로그인 정보를 Login Server로 전송합니다.
   
2. Login Server는 유저의 게임 시작 요청을 수신하면, MySQL에서 유저 정보와 인벤토리 데이터를 가져와 Redis Cluster에 로드합니다.

3. 모든 데이터를 Redis Cluster에 정상적으로 로드한 후, JWT 토큰을 생성합니다.

4. 생성된 JWT 토큰을 Redis Cluster에 저장하고, 동시에 유저에게 전송합니다.

5. 유저는 전달받은 JWT 토큰으로 중앙 서버에 접속 요청을 전송합니다.

6. 중앙 서버는 Redis Cluster에 저장된 JWT 토큰과 유저가 전송한 토큰을 비교하여, 일치하는 경우 접속을 허가합니다.

7. 유저가 로그아웃하면, 중앙 서버는 Redis Cluster에 저장된 유저 데이터를 MySQL과 동기화합니다.

<br> 

- #### Server & Channel Transition
<img src="https://github.com/user-attachments/assets/66656260-fbaf-419b-b0e0-f1f23bda2f88" width="70%" alt="Server   Channel Transition">

1. 유저가 중앙 서버에 연결되면, 서버 선택 페이지로 이동합니다.

2. 서버를 선택하면 중앙 서버로부터 해당 서버의 주소와 JWT 토큰을 받아, 선택한 서버로 연결을 시도합니다.

3. 서버와의 연결이 성공하면, 채널 이동 페이지로 전환됩니다.

4. 유저가 서버 선택 페이지로 돌아갈 때, 접속 중인 서버는 중앙 서버에 유저 퇴장 신호를 보내어 서버 유저 수를 감소시킵니다.
 
5. 유저는 중앙 서버의 유저 수 감소 처리가 완료될 수 있도록 1초 동안 대기합니다.

6. 채널 및 서버 이동 중에도 유저 수를 지속적으로 동기화합니다.

<br> 

- #### Raid Start & Raid End (Mob Hp 0)
<img src="https://github.com/user-attachments/assets/e2c99091-fabb-4900-b39a-fbed30f885d6" width="70%" alt="Raid Start   Raid End (Mob Hp 0)">

1. 레이드가 시작되면, 유저는 서버로부터 참여한 다른 유저들의 정보를 전달받습니다.

2. 전투가 시작되면, 서버는 레이드 상태 동기화를 위해 틱레이트 기반으로 동기화 메시지를 주기적으로 전송합니다.

3. 레이드 몬스터의 HP가 0이 되면 레이드가 종료됩니다.

4. 레이드 종료 후, 유저는 자신의 점수와 다른 유저들의 점수를 확인할 수 있습니다.

5. 서버는 각 유저의 획득 점수가 기존 최고 점수보다 높을 경우, 해당 점수로 랭킹을 업데이트합니다.

<br>

- #### Raid Group Check by Level
<img src="https://github.com/user-attachments/assets/803de230-8b49-434c-9746-e3d4f310c9f9" width="70%" alt="레이드 그룹별">

- 다른 레벨의 그룹이면 매칭이 불가능합니다.

<br>

- #### Equipment Enhancement
<img src="https://github.com/user-attachments/assets/3dc8088e-f5b7-47d5-bef0-d6fe364b13a1" width="70%" alt="장비강화">

- 현재 강화 수치에 따라 확률을 차등 적용하여, 강화 성공 또는 실패를 반환합니다.

<br>

- #### 📺 MMO 서버 프로젝트 전체 동작 시연
> 썸네일을 클릭하면 유튜브 시연 영상으로 이동합니다.

<a href="https://youtu.be/OrwSciNoCAc">
  <img src="https://img.youtube.com/vi/OrwSciNoCAc/maxresdefault.jpg"
       alt="MMO 서버 프로젝트 시연"
       width="500">
</a>

<br>

<br>

## Performance Improvements

코드 검토 또는 부하 테스트 중 발견한 개선점을 적용하고,   
Python 기반 load test client로 기존과 동일한 조건의 부하 테스트를 통해      
안정성과 응답 성능 변화를 측정해 개선 효과를 검증합니다.

<br>

### Optimization #1 — AcceptQueue Lock-free Queue 도입

---

### Problem
다수의 워커 스레드가 AcceptQueue에 동시 접근(push/pop)하는 구조에서    
Mutex 락 비용이 짧은 임계구역 대비 부담일 가능성이 있음.

Lock-free queue가 적합해 보였으나, 직관에 의존한 결정을 지양하고    
측정 기반으로 두 방식의 실제 성능 차이를 확인하고자 함.

---

### Solution

#### Before — Mutex + std::queue
```cpp
std::mutex m1;
std::queue AcceptQueue;

{
    std::lock_guard lg(m1);
    AcceptQueue.push(connUser);
}
```

#### After — Lock-free queue (boost)
```cpp
boost::lockfree::queue AcceptQueue;
AcceptQueue.push(connUser);  // CAS 기반 atomic 연산
```

<br>

같은 인터페이스를 유지하기 위해 두 구현 모두 push/pop 시그니처를    
동일하게 맞춰서 사용처 코드는 변경하지 않고 헤더만 교체하여 비교 가능하도록 구성.

---

### Load Test Result

**조건**: 서버 워커 4개 / 클라이언트 동시성 4~128 / 1000 connections × 5 runs

| Concurrency | Mutex (ms) | Lock-free (ms) | 개선 |
|---|---|---|---|
| 4 | 726 | 563 | -22.5% |
| 8 | 748 | 521 | -30.3% |
| 16 | 775 | 556 | -28.3% |
| 32 | 815 | 554 | -32.1% |
| 64 | 789 | 549 | -30.5% |
| 128 | 748 | 705 | -5.8% |

- 4\~64 구간에서 일관되게 20\~32% 처리 시간 단축
- 32 threads 기준 TPS 1227 → 1807 (+47%)
- 128 구간에서 격차 축소 — Lock-free 또한 극한 경합에서 CAS 재시도 비용 누적
- 측정 결과를 기반으로 패킷 처리 매니저 큐에도 동일 패턴 적용
