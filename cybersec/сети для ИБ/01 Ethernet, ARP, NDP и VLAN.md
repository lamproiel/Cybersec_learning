- L2 часто недооценивают. Все хотят сразу к TCP, HTTP, TLS, сканам, C2 и красивым строкам в pcap. Потом появляется VLAN, bridge или ARP-аномалия, и разбор начинает плыть.
	- PCAP — это файл с захваченными сетевыми пакетами. Название происходит от packet capture.

- Ethernet, ARP и VLAN надо понимать не ради теории. Без них тяжело нормально читать локальный трафик, разбирать MITM-сценарии, pcap-задачи, контейнерные сети, VXLAN и любые истории, где важна точка наблюдения.
## Ethernet

- Ethernet-кадр в нормальном упрощении:
	- dst MAC
	- src MAC
	- EtherType
	- payload 

- EtherType говорит, что лежит внутри:
	- 0x0800 - IPv4
	- 0x0806 - ARP
	- 0x86dd - IPv6
	- 0x8100 - 802.1Q VLAN tag

- MAC-адрес не отвечает за “доставку по интернету”. Он нужен для доставки внутри текущего L2-сегмента, до следующего L2-hop.
	- Пример: хост `192.168.1.50` идет на `8.8.8.8`.

- На первом участке может быть так:
	- Ethernet dst: MAC gateway
	- Ethernet src: MAC client
	- IP dst: 8.8.8.8
	- IP src: 192.168.1.50

- IP назначения удаленный, но MAC назначения локальный, потому что кадр отправляется на gateway.
### Broadcast, multicast, unicast

- На L2 есть несколько типов доставки.
	- unicast   - конкретный MAC
	- broadcast - ff:ff:ff:ff:ff:ff
	- multicast - группа получателей

- ARP-запрос обычно broadcast. ARP-ответ обычно unicast.

- В pcap это важно, потому что broadcast часто показывает локальный контекст. Если хост спрашивает “who has gateway”, это уже хороший маркер, где он находился и с кем пытался общаться.
## ARP

- ARP отвечает на вопросы:
	- Какой MAC соответствует этому IPv4-адресу в моем L2-сегменте?

- Пример:
	- Who has 192.168.1.1? Tell 192.168.1.50
	- 192.168.1.1 is at aa:bb:cc:dd:ee:ff

- Посмотреть neighbor table:
	- `ip neigh`

- Посмотреть только reachable/stale записи:
	- `ip neigh show nud reachable`
	- `ip neigh show nud stale`

- поймать ARP:
	- `sudo tcpdump -ni eth0 arp`

- Wireshark-фильтр:
	- `arp`
### ARP cache и состояния

- В Linux neighbor entry может быть не просто “есть” или “нет”. У нее есть состояние.

- частые состояния:
	- INCOMPLETE - идет резолв, ответа еще не было
	- REACHABLE  - сосед недавно подтвержден
	- STALE      - запись есть, но свежесть уже сомнительная
	- DELAY      - ядро ждет подтверждения
	- PROBE      - идет проверка соседа
	- FAILED     - сосед не отвечает
	- PERMANENT  - статическая запись

- INCOMPLETE обычно живет недолго, запись либо доходит до REACHABLE, либо падает в FAILED.

- Это полезно при диагностике. Например, IP вроде правильный, route есть, но neighbor entry уходит в `FAILED`. Значит проблема может быть не на TCP/HTTP, а ниже, на L2/ARP.
### Gratuitous ARP

- Gratuitous ARP это ARP, где хост обьявляет свой адрес без обычного запроса.

- он встречается при:
	- обновлении ARP cache у соседей;
	- failouver;
	- HA-схемах;
	- смене MAC за IP;
	- некоторых проверках конфликтов IP.

- в ИБ его стоит замечать. Он может быть нормальной инфраструктурной вещью, а может быть частью подозрительной L2-истории.

## IPv6 и Neighbor Discovery

- в IPv6 нет ARP. Его роль выполняет NDP, Neighboor Discovery Protocol, и работает он через ICMPv6.

- основные сообщения:
	- Neighbor Solicitation - аналог ARP request, кто здесь с этим адресом
	- Neighbor Adverisement - аналог ARP reply
	- Router Solicitation - запрос параметров сети у роутера
	- Router Advertisement - роутер сообщает префикс, gateway, фоаги автоконфигурации

- главное отличие от ARP в том, что вместо broadcost тут multicast. Neighbor Solicitation уходит не на ff:ff:ff:ff:ff:ff, а на solicited-node multicast адрес вида ff02::1:ffxx:xxxx, который строится из последних 24 бит искомого адреса.

- посмотреть IPv6 neighbor table:
	- `ip -6 neigh`

- состояние там те же самые, INCOMPLETE, REACHABLE, STALE, DELAY, PROBE, FAILED, PERMANENT. Это одна и та же подсистема ядра, просто для другого протокола. 

- поймать NDP: 
	- `sudo tcpdump -ni eth0 icmp6`

- Wireshark:
	- `icmpv6.type == 135 || icmpv6.type == 136`

- 135 это Neighbor Solicitation, 136 это Neighbor Advertisement.

- еще момент, который путает на старте: у хоста почти всегда есть link-local адрес из диапазона fe80::/10, даже если IPv6 формально не настроен руками. NDP и часть служебного трафика работает именно через него, и в pcap это норма, а не повод что-то подозревать.

## VLAN

- VLAN это L2-разделение.

- не надо путать VLAN и подсеть. Часто делают так:
	- `VLAN 10 -> 192.168.10.0/24`
	- `VLAN 20 -> 192.168.20.0/24`

- из-за этого кажется, что VLAN это "подсеть". Но это разные уровни. VLAN разделяет L2 broadcast domain. Подсеть описывание L3-адресацию.

### 802. 1Q tag

- обычные VLAN tag 802.1Q содержит несколько полей. Самое известное - VLAN ID.
	- PCP - приоритет
	- DEI - drop eligible
	- VID - VLAN ID

- VLAN ID обычно в диапазоне 1-4094. в pcap нас чаще всего интересует именно vlan.id.

- фильтры:
	- `tshark -r traffic.pcapng -Y "vlan"`
	- `tshark -r traffic.pcapng -Y "vlan" -T fields -e frame.number -e vlan.id -e eth.src -e eth.dst`

### Access и trunk 

- Access port обычно отдает утройству трафик одной VLAN без видимого tag.

- Trunk port несет несколько VLAN, поэтому tag нужен.

- если capture снят на access-порту, VLAN tag может отсутствовать.

- если capture снят на trunk или SPAN, ты можешь видеть VLAN ID.

- это прямо влияет на анализ. Один и тот же сетевой эпизод в разных точках может выглядеть по-разному.

## Где здесь ИБ

- L2 нужен в таких местах:
	- ARP spoofing и MITM как концепт;
	- поиск gateway;
	- понимание broadcast domain;
	- анализ pcap;
	- VLAN-контекст в CTF;
	- container networking;
	- bridge/veth;
	- VXLAN, где inner Ethernet переносится поверх L3;
	- диагностика “IP есть, но ничего не ходит”.

## Ошибка, которая часто ломает разбор

- человек видит строку в pcap и сразу берет ее как ответ. Но строка может быть в неправильном VLAN.

- нормальный разбор должен сначало ответить:
	- В каком VLAN это было?
	- Кто source MAC?
	- Кто destination MAC?
	- Был ли ARP до этого?
	- Это access-контекст или trunk/SPAN?
	- Есть ли такой же похожий поток в другом VLAN?

- только потом стоит читать payload.

## Мини-чеклист по L2

1. Есть ли VLAN tags?
2. Какие vlan.id встречаются?
3. Какие MAC чаще всего общаются?
4. Есть ли broadcast?
5. Есть ли ARP?
6. Кто искал gateway?
7. Есть ли gratuitous ARP?
8. Видно ли несколько L2-контекстов?
9. Не выглядит ли часть трафика как decoy?