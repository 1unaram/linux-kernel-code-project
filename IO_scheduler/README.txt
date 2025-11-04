- block/elevator.c - 커널의 일반적인 I/O 엘리베이터 코드로, rbtree를 활용하여 요청을 추적합니다. 이 파일에는 elv_rb_find() 함수 등 rbtree 관련 함수들이 포함되어 있습니다.
- block/deadline-iosched.c - Deadline I/O 스케줄러 구현 파일입니다. 읽기와 쓰기를 위한 별도의 rb tree를 유지하여 요청을 섹터 위치별로 정렬하여 관리합니다.
- block/cfq-iosched.c - Completely Fair Queuing(CFQ) I/O 스케줄러 구현 파일입니다. 각 프로세스마다 작업 큐를 관리하기 위해 rbtree를 사용합니다.


구체적인 rbtree 사용 코드
Deadline 스케줄러에서는 다음과 같이 rbtree를 구성합니다.​
- 읽기 rb tree (섹터로 정렬)
- 읽기 FIFO 리스트 (큐 시간으로 정렬)
- 쓰기 rb tree (섹터로 정렬)
- 쓰기 FIFO 리스트 (큐 시간으로 정렬)
- 요청 해시 테이블 (끝 섹터로 정렬)
이러한 구조를 통해 I/O 요청을 효율적으로 추적하고 디스크 헤드 이동을 최소화합니다.