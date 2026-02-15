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

<br> 

#### ㅇ 개발 기간 
- 2025.01.14 ~ 2025.02.28 : IOCP & Redis Cluster를 활용한 MMO 게임 서버 개발  
- 2025.04.06 ~ 2025.04.22 : 서버 분할 구조 도입 (Center Server를 4개로 분할)
- 2025.07.15 ~ 2025.07.30 : 아이템/상점/패스 콘텐츠 추가

#### ㅇ 프로젝트 소개서 - [MMOServer_Project_Intro.pdf](https://github.com/user-attachments/files/22309430/MMOServer_Project_Intro.pdf)




<br>  

## [Flow Chart]


- #### User Connect
![Game Server Connect drawio](https://github.com/user-attachments/assets/efa77c95-aed4-487f-9d56-eb2d21af1d27)

<br>

- #### Move Server
![Move Channel drawio](https://github.com/user-attachments/assets/ab0ea345-f916-4371-a4da-82483e784ca8)

<br>

- #### Raid Matching
![Raid Start drawio](https://github.com/user-attachments/assets/f767bc79-8db3-4c6e-820b-d6b9a1d119b7)


<br>
<br>

## [Test Output]

> 썸네일을 클릭하면 유튜브 시연 영상으로 이동합니다.

<br>

#### 📺 MMO 서버 프로젝트 전체 동작 시연


[![MMO 서버 프로젝트 시연](https://img.youtube.com/vi/fpL2SLlo7qA/0.jpg)](https://www.youtube.com/watch?v=fpL2SLlo7qA&t=49s)




