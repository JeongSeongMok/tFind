# tFind
### 멀티쓰레딩을 이용한 키워드 검색 프로그램
---
전에 만들었던 [pFind](http://www.github.com/JeongSeongMok/pFind)(멀티 프로세싱을 이용한 키워드 검색)와 비슷하지만 멀티 쓰레딩을 이용함.

설명 링크 : https://youtu.be/AA6_V7q2Ws4

> 사용법
> - `make`
> - `./pfind <쓰레드 개수> <디렉토리 이름> <keywords+>`
> - 쓰레드 개수 16개 이하, keywords 개수 8개 이하

> ex) `./pfind 8 ./dir1 abc file`