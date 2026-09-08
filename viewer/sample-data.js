// Generated from examples/ver5-sample/results.json.
window.SCHEDULER_SAMPLE = {
  "schema_version": 1,
  "simulator_version": "5.0.0",
  "time_unit": "ticks",
  "assumptions": {
    "cpu_count": 1,
    "context_switch_cost": 0,
    "io_wait": false
  },
  "input": [
    {
      "id": 1,
      "arrival": 0,
      "burst": 5
    },
    {
      "id": 2,
      "arrival": 1,
      "burst": 3
    },
    {
      "id": 3,
      "arrival": 2,
      "burst": 1
    }
  ],
  "runs": [
    {
      "algorithm": "FCFS",
      "quantum": null,
      "policy": "arrival_then_input",
      "processes": [
        {
          "id": 1,
          "arrival": 0,
          "burst": 5,
          "first_start": 0,
          "completion": 5,
          "waiting": 0,
          "turnaround": 5,
          "response": 0
        },
        {
          "id": 2,
          "arrival": 1,
          "burst": 3,
          "first_start": 5,
          "completion": 8,
          "waiting": 4,
          "turnaround": 7,
          "response": 4
        },
        {
          "id": 3,
          "arrival": 2,
          "burst": 1,
          "first_start": 8,
          "completion": 9,
          "waiting": 6,
          "turnaround": 7,
          "response": 6
        }
      ],
      "summary": {
        "average_waiting": 3.3333333333333335,
        "average_turnaround": 6.333333333333333,
        "average_response": 3.3333333333333335,
        "makespan": 9
      },
      "timeline": [
        {
          "process_id": 1,
          "start": 0,
          "end": 5
        },
        {
          "process_id": 2,
          "start": 5,
          "end": 8
        },
        {
          "process_id": 3,
          "start": 8,
          "end": 9
        }
      ]
    },
    {
      "algorithm": "SJF",
      "quantum": null,
      "policy": "burst_then_arrival_then_input",
      "processes": [
        {
          "id": 1,
          "arrival": 0,
          "burst": 5,
          "first_start": 0,
          "completion": 5,
          "waiting": 0,
          "turnaround": 5,
          "response": 0
        },
        {
          "id": 3,
          "arrival": 2,
          "burst": 1,
          "first_start": 5,
          "completion": 6,
          "waiting": 3,
          "turnaround": 4,
          "response": 3
        },
        {
          "id": 2,
          "arrival": 1,
          "burst": 3,
          "first_start": 6,
          "completion": 9,
          "waiting": 5,
          "turnaround": 8,
          "response": 5
        }
      ],
      "summary": {
        "average_waiting": 2.6666666666666665,
        "average_turnaround": 5.666666666666667,
        "average_response": 2.6666666666666665,
        "makespan": 9
      },
      "timeline": [
        {
          "process_id": 1,
          "start": 0,
          "end": 5
        },
        {
          "process_id": 3,
          "start": 5,
          "end": 6
        },
        {
          "process_id": 2,
          "start": 6,
          "end": 9
        }
      ]
    },
    {
      "algorithm": "RR",
      "quantum": 2,
      "policy": "arrivals_before_requeue",
      "processes": [
        {
          "id": 1,
          "arrival": 0,
          "burst": 5,
          "first_start": 0,
          "completion": 9,
          "waiting": 4,
          "turnaround": 9,
          "response": 0
        },
        {
          "id": 2,
          "arrival": 1,
          "burst": 3,
          "first_start": 2,
          "completion": 8,
          "waiting": 4,
          "turnaround": 7,
          "response": 1
        },
        {
          "id": 3,
          "arrival": 2,
          "burst": 1,
          "first_start": 4,
          "completion": 5,
          "waiting": 2,
          "turnaround": 3,
          "response": 2
        }
      ],
      "summary": {
        "average_waiting": 3.3333333333333335,
        "average_turnaround": 6.333333333333333,
        "average_response": 1,
        "makespan": 9
      },
      "timeline": [
        {
          "process_id": 1,
          "start": 0,
          "end": 2
        },
        {
          "process_id": 2,
          "start": 2,
          "end": 4
        },
        {
          "process_id": 3,
          "start": 4,
          "end": 5
        },
        {
          "process_id": 1,
          "start": 5,
          "end": 7
        },
        {
          "process_id": 2,
          "start": 7,
          "end": 8
        },
        {
          "process_id": 1,
          "start": 8,
          "end": 9
        }
      ]
    }
  ]
};
