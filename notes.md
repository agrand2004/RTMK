Should we pop the NextTask from the ReadyList on run()?

TODO:
- Create a list function exception insert_before(list, obj);
- in create_task:
    set TCB PC to point at the taskbody
    set up stack frame
    set TCB’s SP to point to the correct cell in stack segment
    create new_obj with TCB information
    do the else statement