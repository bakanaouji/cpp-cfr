# Open CFR
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
In order to compute an approximate Nash equilibrium in a imperfect information game, execute the following commands:

```bash
$ cd RegretMinimization
$ sh build.sh
$ ./build/RegretMinimization
```

Once complete, you should have a binary file that represents the average strategy, like `strategies/kuhn/strategy_cfr.bin`.

### Evaluating average strategies
If you have finished computing an approximate Nash equilibrium, you can compute the expected payoff of the obtained average strategy.

```bash
$ cd Game
$ sh build.sh
$ ./build/Game
```

As a result of executing these commands, the command-line interface will output the expected payoff of each player:

```bash
expected payoffs: (-0.0555557,0.0555557,)
```

## References
[1] Martin Zinkevich, Michael Johanson, Michael Bowling, and Carmelo Piccione. Regret minimization in games with incomplete information. In Advances in neural information processing systems, pp. 1729–1736, 2008.

[2] Marc Lanctot, Kevin Waugh, Martin Zinkevich, and Michael Bowling. Monte carlo sampling for regret minimization in extensive games. In Advances in neural information processing systems, pp. 1078–1086, 2009.