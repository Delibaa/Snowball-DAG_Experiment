import concurrency_simulation

if __name__ == '__main__':

    # parameter setting
    fork_coefficient = 500
    transaction_pool_capacity = 1000
    concurrent_blocks_interval = 10
    block_transmission_interval = 2
    # test
    # transaction_pool_capacity = 10

    print("The setting: ")
    print("The transaction rate of " + str(transaction_pool_capacity / 30) + " tx/s")
    print("The fork coefficient in the network of " + str(fork_coefficient))
    print("The block size is 20K")
    print("The round interval is " + str(concurrent_blocks_interval) + "s")
    print("The block transmission interval is " + str(block_transmission_interval) + "s")
    print("————————————————————————————————————————————————————————————————————————————")

    # Initializing the transaction pool, with 100% synchronous txs
    transaction_pool = concurrency_simulation.Transaction_pool(1, transaction_pool_capacity)
    for i in range(0, transaction_pool_capacity):
        transaction_pool.txs.append(concurrency_simulation.Transaction().create_one_transaction())

    # simulation
    blocks1 = concurrency_simulation.package_simulation_strategy1(transaction_pool, fork_coefficient)
    blocks2 = concurrency_simulation.package_simulation_strategy2(transaction_pool, fork_coefficient)
    blocks3 = concurrency_simulation.package_simulation_strategy3(transaction_pool, fork_coefficient)

    # calculation effective transaction rate
    Concurrency_transactions_of_strategy1 = []
    for block in blocks1:
        Concurrency_transactions_of_strategy1.extend(block.txs)
    Effective_transactions_of_strategy1 = set(Concurrency_transactions_of_strategy1)
    C_Txs_1 = len(Concurrency_transactions_of_strategy1)
    E_Txs_1 = len(Effective_transactions_of_strategy1)
    E_rate_1 = "%.4f" % (E_Txs_1 / C_Txs_1)
    print("The Conflux package according to tx fees: ")
    print("The concurrency Transactions of Conflux is " + str(C_Txs_1))
    print("The effective transaction rate of Conflux is " + E_rate_1)
    print("************************************************************************")

    Concurrency_transactions_of_strategy2 = []
    for block in blocks2:
        Concurrency_transactions_of_strategy2.extend(block.txs)
    Effective_transactions_of_strategy2 = set(Concurrency_transactions_of_strategy2)
    C_Txs_2 = len(Concurrency_transactions_of_strategy2)
    E_Txs_2 = len(Effective_transactions_of_strategy2)
    E_rate_2 = "%.4f" % (E_Txs_2 / C_Txs_2)
    print("The Prism package with choosing txs randomly: ")
    print("The concurrency Transactions of Prism is " + str(C_Txs_2))
    print("The effective transaction rate of Prism is " + E_rate_2)
    print("******************************************************************")

    Concurrency_transactions_of_strategy3 = []
    for block in blocks3:
        Concurrency_transactions_of_strategy3.extend(block.txs)
    Effective_transactions_of_strategy3 = set(Concurrency_transactions_of_strategy3)
    C_Txs_3 = len(Concurrency_transactions_of_strategy3)
    E_Txs_3 = len(Effective_transactions_of_strategy3)
    E_rate_3 = "%.4f" % (E_Txs_3/C_Txs_3)
    print("The OHIE package with choosing txs mapping to its miner's address: ")
    print("The concurrency Transactions of OHIE is " + str(C_Txs_3))
    print("The effective transaction rate of OHIE is " + E_rate_3)
