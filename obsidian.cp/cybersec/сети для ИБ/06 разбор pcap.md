sPcap не стоит начинать со strings. Это полезная команда, но плохой первый шаг. Она может показать читаемый мусор, decoy или кусок payload без контекста.
- strings - стандартная консольная утилита для сканирования файлов и вывода из него читаемые строки текста.
- decoy - специально подброшенные данные или пакеты, призванные запутать аналитика или отвлечь внимание системы защиты.
- payload — это полезная нагрузка пакета, то есть данные, которые пакет реально переносит.

# шаг 1. общая информация

`capinfos traffic.pcapng`
смотреть: 
- формат
- количество пакетов
- время первого и последнего пакета
- длительность
- snaplen - максимальный размер пакета в байтах, который захватывается в файл
- interfaces - список сетевых адаптеров, с которых производится перехват трафика
- comments - текстовые заметки и аннотации, встроенные прямо в структуку файла.

если capture короткий, это один тип анализа. Если там тысячи пакетов за несколько часов, нужен другой подход.

## время,  часовые пояса и рассинхрон 

если в задаче несколько capture с разных точек, например с клиента и с gateway, не верь абсолютным таймстампап сразу. Часы на разных хостат редко синхронизированы идеально, расхождение на пару секунд это норма, а не аномалия.

надеждее искать общий якорь, один и тот же пакет или один и тот же TCP handshake, видимый в обоих capture, и мерить расхождение относительно него, а не относительно часов на стене.

capinfos показывает время в той зоне, которую считает нужным конкретный билд, поэтому если что-то не сходится по времени, первым делом проверить таймзону, а не сразу делать выводы про порядок событий.

## шаг 2. protocol hierarchy

`tshark -t traffic.pcapng -g -z io,phs`
- так быстро видно, что вообще есть внутри: Ethernet, VLAN, IPv4, IPv6, ARP, TCP, UDP, DNS, HTTP, TLS, ICMP, VXLAN и так далее

## шаг 3. endpoints и conversations

endpoint - кто в сети. Это отдельное устройство (IP или MAC-адрес), которое что-то отправляло или принимало.

conversation - диалог между двумя устройствами. Связь формата узла А и узла Б, показывающая, кто с кем общался и сколько данных они друг другу передали.

`tshark -r traffic.pcapng -q -z endpoints,eth`
`tshark -r traffic.pcapng -q -z endpoints,ip`
`tshark -r traffic.pcapng -q -z conv,ip`
`tshark -r traffic.pcapng -q -z conv,tcp`
`tshark -r traffic.pcapng -q -z conv,udp`
искать: 
- кто больше всего общался
- есть ли один явный client
- есть ли один явный server 
- есть ли gateway
- есть ли внешний IP
- есть ли шум
- есть ли похожие потоки

## шаг 4. L2

`tshark -r traffic.pcapng -Y "arp"`
`tshark -r traffic.pcapng -Y "vlan"`
`tshark -r traffic.pcapng -T fields -e frame.number -e eth.src -e eth.dst -e eth.type | head`

если есть VLAN:
`tshark -r traffic.pcapng -Y "vlan" -T fields -e frame.number -e vlan.id -e eth.src -e eth.dst`

здесь часто видно, какой поток реально относится к intended-контексту, а какой просто похож.
- intended-контекст - реальный поток событий или данных, который вы ищете по задаче.

## шаг 5. L3

`tshark -r traffic.pcapng -Y "icmp"`
`tshark -r traffic.pcapng -Y "ip.flags.mf == 1 || ip.frag_offset > 0"`

смотреть ICMP errors, TTL, fragmentation, странные маршруты.
- fragmentation - процесс разбиение одного большого пакета на несколько более мелкхи частей.

## шаг 6. TCP/UDP

`tshark -r traffic.pcapng -Y "tcp.flags.syn == 1"`
`tshark -r traffic.pcapng -Y "tcp.flags.reset == 1"`
`tshark -r traffic.pcapng -Y "tcp.analysis.retransmission"`

для UDP смотреть endpoints, размеры, timing и application layer

### чексуммы, которые битые, и это нормально

если Wireshark помечает исходящий пакет с твоей же машины как bad checksum, это почти всегда не баг и не атака. Современные сетевые карты считают checksum сами на этапе отправки, уже после того как пакет ушел в capture. Wireshark видит пакет до этого момента, поле еще не посчитано, и честно помечает его как неверное.

отличить реальную проблему от offlad просто. Если bad checksum только на исходящих пакетах с локального интерфейса, а соединение при этом работает нормально, это offload. Если bad checksum на входящих пакетах, или на трафике, который реально не доходит, тогда стоит присмотреться внимательнее.
- offload - перенос сетевой обработки (подсчёт чексумм, нарезка TCP пакетов) с CPU на сетевую карту.

в Wireshark это можно выключить, чтобы не отвлекало: Edit - Preferences - Protocols - выбрать IPv4, TCP или UDP, и снять галочку с проверки checksum.

## шаг 7. DNS/HTTP/TLS

`tshark -r traffic.pcapng -Y "dns"`
`tshark -r traffic.pcapng -Y "http.request"`
`tshark -r traffic.pcapng -Y "tls.handshake.extensions_server_name"`

только после этого TCP streams, objects, payload и readable strings.
- TCP streams - упорядоченный диалог между 2 приложениями в рамказ одного TCP-соединения (от SYN до FIN,RST)
- objects - файлы, автоматически вытащенные из трафика (скачанные exe, картинки, php-скрипты через HTTP/SMB)
- readable strings - осмысленный текст (токены, пароли, команды), найденный внутри целевого потока или извлечённого файла.

## когда можно использовать strings

strings - базовая утилита командной строки, предназначенная для поиска и извлечения человекочитаемого текста из любых файлов.

можно, но лучше после базового контекста.
	`strings traffic.pcapng | less`

если строка нашлась, надо проверить:
- в каком frame она была (frame - отдельный пакет с порядковым номером)
- в каком stream
- в каком VLAN
- кто source/destination
- был ли нормальный TCP state - состояние соединения.
- не было ли похожего decoy - ложный след, фейковый пакет или приманка.

## формат аккуратного вывода

Fact:
- Capture содержит VLAN 314 и VLAN 777.
- Во VLAN 314 есть ARP-запрос от host A к gateway.
- После ARP host A открывает TCP-соединение к host B.
- Payload появляется после успешного handshake.

Probable:
- VLAN 314 является intended-контекстом, потому что порядок событий совпадает с сетевой логикой.

Possible:
- VLAN 777 может быть decoy, так как там есть похожий payload, но порядок событий не совпадает.

такой вывод выглядит спокойнее и сильнее, чем "я нашел строку, значит вот ответ"

# короткий pcap-чеклист

1. capinfos
2. protocol hierarchy
3. endpoints
4. conversations
5. ARP/VLAN
6. ICMP/fragmentation
7. TCP states
8. DNS
9. HTTP/TLS
10. payload
11. decoy check
12. fact/probable/possible