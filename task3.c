/*

To do

1. Cirl + Z 
    foreground job이 멈추어야 한다.
        *만약 jobs을 내가 구현해야 한다면, 어떤 fg job이 stopped 되었을 떄 이것의 상태와 pid, process name 을 list에 저장해야 한다. 
    myshell은 ignore 해야 함.  
2. CIrl + C 
    - Foreground job이 종료되어야 한다(SIGINT) 
    - myshell은 멈추지 말아야 한다: ignore


3. jobs: 직접 구현해야 하는가? 
    - Role: list running or stopped background job
        *foreground job은 list하지 않는 이유: fg job이 running하고 있을 때는 다른 커맨드를 입력할 수 없다. 


4. bg fg
    bg: stopped bg job --> running bg job 

        SIGCONT 신호를 해당 job에게 줌.
        

    fg: stopped or running bg --> running fg job 


5. kill



*/