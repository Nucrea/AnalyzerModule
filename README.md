# AnalyzerModule

Проект для мехатронных модулей автоматизированного анализатора культи конечности.

Устройство состоит из девяти независимых модулей, управляемых скриптом-координатором. 
Каждый модуль имеет привод с шаговым двигателем и винтовой передачей, и тензометрические датчики. По команде, происходит измерение формы культи и твердости тканей.
Общение с модулями производится через UART, доступны внешние команды управления (движение, остановка, установка коэффициентов и скоростей).

![Alt text](https://i.ibb.co/0Vy8VQg/gubb9-ZLWteo.jpg)
