# Cpp CFR
This repository is C++ implementations of Counterfactual Regret Minimization (CFR) [1] algorithms for an extensive-form game with imperfect information.
CFR is an iterative algorithm that is proven to converge to a Nash equilibrium in two-player zero-sum games.
Currently, this repository provides the following variants of CFR implementation:

* vanilla CFR [1]
* Chance-Sampling Monte Carlo CFR [2]
* External-Sampling Monte Carlo CFR [2]
* Outcome-Sampling Monte Carlo CFR [2]

## Dependencies
* C++ compiler supporting C++14
* [CMake](https://cmake.org/) 3.5 or higher
* [Boost](https://www.boost.org/) 1.68.0 or higher

## Getting start
### Computing an approximate Nash equilibrium
In order to compute an approximate Nash equilibrium in an imperfect information game, first, you should build the project:

```bash
$ sh build.sh
$ cd build
```

Next, execute the following commands to run an experiment:

```bash
$ ./RegretMinimization/run_cfr --iteration=1000000
```

This command requires an argument that specifies the number of iterations of CFR.

In addition to this argument, the following options can be selected:

* `-a, --algorithm`: A variant of CFR algorithm computing an equilibrium. You must specify `"vanilla"`, `"chance"`, `"external"` or `"outcome"`. The default value is `"vanilla"`.
* `-i, --iteration`: A number of iterations of CFR.
* `-s, --seed`: A random seed used to initialize the random generator. If you don't specify this argument, the random seed will be set to the random number generated by `std::random_device`.

Once the experiment is completed, you should have a binary file that represents the average strategy, like `strategies/kuhn/strategy_vanilla.bin`.

### Evaluating average strategies
If you have finished computing an approximate Nash equilibrium, you can compute the expected payoffs and exploitability of the obtained average strategy profile:

```bash
$ ./Game/game --strategy-path-0="../strategies/kuhn/strategy_vanilla.bin" --strategy-path-1="../strategies/kuhn/strategy_vanilla.bin"
```

This command requires arguments that the paths to the binary files that represent the average strategies for players.
You should specify the path for each player as `strategy-path-0`, `strategy-path-1` and so on.

As a result of executing these commands, the command-line interface will output the expected payoffs and exploitability of the given strategy profile:

```bash
expected payoffs: (-0.0555556,0.0555556,)
exploitability: 6.59955e-06
```

### Running with Docker
This repository also provides `Dockerfile` for docker users.
If you want to run with Docker, build the container:

```bash
$ docker build -t cpp-cfr .
```

After build finished, run the container:

```bash
$ docker run -it cpp-cfr
```


## References
[1] Martin Zinkevich, Michael Johanson, Michael Bowling, and Carmelo Piccione. Regret minimization in games with incomplete information. In Advances in neural information processing systems, pp. 1729–1736, 2008.

[2] Marc Lanctot, Kevin Waugh, Martin Zinkevich, and Michael Bowling. Monte carlo sampling for regret minimization in extensive games. In Advances in neural information processing systems, pp. 1078–1086, 2009.
