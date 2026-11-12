# GitStat Analyzer

Учебный проект на C: анализ статистики по контрибьюторам из CSV двумя
способами - через неименованные каналы (`pipe`) и через именованные (`FIFO`).

Обрабатываются записи с `aux >= 3`. Для каждого автора считается количество
записей, сумма и максимум `val`, затем определяется лидер по сумме.

## Формат данных

`gitstat.csv` - заголовок и строки `id;key;val;aux`:

```csv
id;key;val;aux
CMT101;ivanov;1113;11
CMT102;kuznetsova;793;2
CMT103;sidorov;6;3
```

## Структура

```
p1_gitstat.c    # часть 1: полный цикл на pipe
gen_gitstat.c   # часть 2: читает CSV, пишет в FIFO
flt_gitstat.c   # часть 2: фильтр aux >= 3, пишет в FIFO
rep_gitstat.c   # часть 2: агрегация, сортировка, отчёт
gitstat.csv     # данные
Makefile
scripts/run_fifo.sh
```

## Сборка

Нужны Linux, `gcc` и `make`. Запускать программы из корня проекта.

```bash
make          # всё в build/
make clean
```

## Часть 1: pipe

```bash
make run-p1
```

Родитель находит уникальные ключи, создаёт по два канала на ключ и по процессу
на ключ. Потомок фильтрует строки, считает `count`/`sum`/`max`, пишет в канал
`key;count;sum;max` и завершается с кодом `count`. Родитель собирает
результаты, проверяет `count` по коду завершения и выбирает лидера.

```text
ivanov	2	1928	1113
kuznetsova	2	2042	1155
sidorov	1	6	6
smirnov	1	29	29
petrova	1	932	932
leader: kuznetsova (sum=2042)
```

## Часть 2: FIFO

```bash
make run-fifo
```

`gen_gitstat` → `/tmp/gitstat_g2f.fifo` → `flt_gitstat` →
`/tmp/gitstat_f2r.fifo` → `rep_gitstat`. Порядок запуска не важен: процессы
блокируются на `open()`, пока не появится вторая сторона. `flt` передаёт
число отфильтрованных строк как `DROPPED n`, `rep` печатает отчёт и удаляет
свои FIFO.

```text
ivanov	2	1928
kuznetsova	2	2042
petrova	1	932
sidorov	1	6
smirnov	1	29
total=7, dropped=7; всего строк: 14; сумма по всем ключам: 4937
```

Вручную: запустить `rep_gitstat`, `flt_gitstat` и `gen_gitstat` в трёх
терминалах.

## Ограничения

- `count` передаётся кодом завершения, а он усечён до 8 бит: при `count > 255`
  будет `MISMATCH`.
- Лимиты: 32 ключа в части 1, 100 ключей и 256 символов строки в `rep`.
- CSV простой: `;` без кавычек, `val`/`aux` — целые `int`.
- Ключ автора не должен начинаться с `DROPPED`.

## Автор

Шухардин Илья
