## Кэш-оптимальный спинлок
Взглянем на код функций `Acquire` и `Release`.
```
1  AcquireLock()
2    prev_tail = spinlock_.wait_queue_tail_.exchange(this)
3    if (prev_tail != nullptr)
4      prev_tail->next_ = this
5      while (!is_owner_)
6        передать управление другому процессу
7
8  ReleaseLock()
9   this_ptr_ = this
10   if (!spinlock_.wait_queue_tail_.compare_exchange_strong(this_ptr_,
11                                                           nullptr))
12     while (next_.load() == nullptr)
13       передать управление другому процессу
14     next_.load()->is_owner_.store(true)
```
В функции `Acquire`, во всех строчках кроме 2, используются данные только одного объекта `LockGuard`. К его данным обращаются только его методы, а значит, промахи по кэшу произойдут константное число раз. Во второй же строке указатель на хвост списка будет загружен атомароно единажды, после чего не понадобится. Тем самым, +1 промах по кэшу.
В функции `Release` тоже не наблюдается циклической подгрузки элементов не принадлежащих конкретному объекту `LockGuard`, поэтому по аналогии здесь тоже будет наблюдаться константное число промахов по кэшу.
Реализации `TestAndSet`, `TestAndTAS` и `Ticket` содержат циклические обращения к общим данным.

- В `Ticket` циклически проверяется текущий номер.
- В `TestAndSet` и `TestAndTAS` циклически проверяется флаг возможности захвата.

Именно из-за этого у них может возникать *ping-pong*, когда несколько разных потоков будут поочерёдно подгружать в кэш общую переменную и вызывать циклические промахи.
*Ping-pong* у нас не наблюдается, так как каждый поток работает с памятью внутри своего объекта класса. Если же обращается к переменным вне него, то делает это не циклически, константное количество раз. Данные каждого потока не упорядочены как в массиве, а разбросаны по памяти, так как мы используем список, поэтому *false-sharing* тоже не наблюдается. Также, мы не подаём никаких знаков для всех потоков разом, поэтому *thundering herd* тоже не возникает.