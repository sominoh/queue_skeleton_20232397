# queue_skeleton_20232397

## 제출자 
- 이름: 오소민
- 학번: 20232397

## 구현 내용 

- 요구된 함수들 (`init`, `release`, `enqueue`, `dequeue`, `range`, `nalloc`, `nclone`, `nfree`) 구현 완료
- thread, mutex, atomic 기능만 사용하여 thread-safe한 우선순위 큐 구현
- 내부 자료구조는 단일 연결리스트 기반, 각 노드는 key 기준 내림차순 정렬
- `enqueue()`와 `dequeue()`에서 각각 `tail_lock` / `head_lock`을 사용하여 동시성 제어
- 메모리 재사용을 위한 freelist 도입
- 다양한 크기의 `value`처리를 위해 `value_size` 기반 복사 사용
- 큐 크기 무결성 검사를 위해 `assert` 추가

