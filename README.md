# Sprints - RTOS
### Author
- Hossam Elwahsh
---
### Assignment: S_RTOS_01
#### Task 1
- Using FreeRTOS and a single task to toggle an LED every 1000ms.

#### Task 2
- Using FreeRTOS to toggle 3 LEDs with different timings.
  - LED 1 toggles every `100ms`
  - LED 2 toggles every `500ms`
  - LED 3 toggles every `1000ms`

#### Task 3
- Using FreeRTOS to control LED toggling rate by pressing a push button. (action on release/falling edge)
	- LED toggles every `100ms` if the button was pressed for more than `4 sec`
	- LED toggles every `400ms` if the button was pressed for more than `2 sec` and less than `4 sec`
	- LED is off if the button was pressed for less than `2 sec`

> #### Combined Video 📽️ 👉 [Youtube](https://youtu.be/PVvxsCqyGT4)

|              | Task 1                                                                 | Task 2                                                                 | Task 3                                                                 |
|--------------|------------------------------------------------------------------------|------------------------------------------------------------------------|------------------------------------------------------------------------|
| Pull Request | [PR-1](https://github.com/HossamElwahsh/sprints-arm-rtos-tasks/pull/1) | [PR-2](https://github.com/HossamElwahsh/sprints-arm-rtos-tasks/pull/2) | [PR-3](https://github.com/HossamElwahsh/sprints-arm-rtos-tasks/pull/3) |

----

### Assignment: S_RTOS_02

#### Task 1
- Using FreeRTOS to toggle an LED when push button is pressed and released.

| Task Item    | Task 1                                                                 | Task 2 | Task 3 |
|--------------|------------------------------------------------------------------------|--------|--------|
| Pull Request | [PR-4](https://github.com/HossamElwahsh/sprints-arm-rtos-tasks/pull/4) | TBC    | TBC    |

--- 
### Common Technical Data
| Item         | --- |         Data         |
|--------------|-----|:--------------------:|
| FreeRTOS     | 👉  |       v10.3.0        | 
| IDE          | 👉  | Keil µVision 5 MDK29 | 
| System clock | 👉  |        12MHz         | 
| RTOS Tick    | 👉  |    1ms - 1000 Hz     | 
