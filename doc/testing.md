# Messages

### `chain`

This message is used to request the blocks in a node's chain. The client will update its chain if needed.

### `newblock`

This message is used to inform the node that another node has mined a new block.

### `summary`

This message is used to get information about the node's copy of the chain, such as the first block, the last block and the cumulative difficulty.

# New Blocks

These scenarios are intended to cover what happens when two nodes are linked and both are actively mining. 

## Scenario 1 

Assume `N1` and `N2` are in sync from #0-#10 and both are mining #11. If `N1` finished first then it must notify `N2` using a `newblock` message. This message needs to contain:

* information about the newblock
* the new cumulative difficulty of the chain

### Scenario 1.1

Since both nodes are in sync, `N2` should:

1. stop mining #11 immediately 
2. add `N1`'s #11 to its local copy of the chain
2. resume mining of #12

## Scenario 2

Assume that `N1` and `N2` are in sync from `#0`-`#9`, but communication is lost. `N1` has blocks `#10`-`#15` and `N2` has blocks `#10'`-`#15'`. When communication is restored, `N1` finishes mining block `#16` and sends a `newblock` message to `N2`

### Scenario 2.1

This is the scenario of when a `newblock` is sent and the **local chain** has the greater cumulative difficulty.

If `N1` has a greater cumalative difficulty then `N1` will continue on mining with `#17`. 

`N2` should:
1. abort mining
2. request a `summary` from `N1`
3. do `chain` update to update chain
4. resume mining

### Scenario 2.2

This is the scenario of when a `newblock` is sent and the **remote chain** has the greater cumulative difficulty.

If `N2` has a greater cumalative difficulty, then `N2` should continue mining. 

`N1` should:
1. abort mining
2. request a `summary` from the 
3. do `chain` update to update chain
4. resume mining

## Scenario 3

`N1` and `N2` are in sync with blocks `#0`-`#9`. `N1` has blocks 
