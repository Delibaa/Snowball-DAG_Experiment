import hashlib
import random
import rsa


# transactions
class Transaction:
    def __init__(self):
        self.hash = ""
        self.transaction_fees = 0

    # create a transaction with random content
    def create_one_transaction(self):
        alphabet = 'abcdefghijklmnopqrstuvwxyz!@#$%^&*()0123456789'
        content = random.sample(alphabet, 20)
        self.hash = hashlib.sha256("".join(content).encode('utf-8')).hexdigest()
        # do a survey of tx fees
        self.transaction_fees = random.randint(1, 100)
        return self

    # create batches transactions
    def create_transactions(self, size):
        batch_txs = []
        for i in range(1, size):
            batch_txs.append(self.create_one_transaction(self))
        return batch_txs

    def send_one_transaction(self, transaction_pool):
        transaction_pool.add_transactions(self.create_one_transaction())

    # def send_transactions(self, transaction_pool, txs):
    #     transaction_pool.add_transactions(txs)


# transaction_pool
class Transaction_pool:
    def __init__(self, number, size):
        self.number = number
        self.txs = []
        self.capacity = size

    # add batches transactions
    def add_transactions(self, txs):
        for tx in txs:
            self.txs.append(tx)

    # getting the current transaction numbers in the pool
    def get_transaction_number(self):
        return len(self.txs)

    # delete batches transactions and fill up the pool
    def delete_transactions(self, txs):
        for tx in txs:
            if tx in self.txs:
                self.txs.remove(tx)
        for i in range(self.get_transaction_number(), self.capacity):
            self.txs.append(Transaction().create_one_transaction())


# block
class Block:
    # using the number replacing the hash
    def __init__(self):
        self.number = 0
        self.time = 1
        self.txs = []
        # test
        self.capacity = 130
        self.miner = ""

    # strategy one: packaging blocks according to the tx fees
    def choose_txs_with_transaction_fees(self, transaction_pool):
        if len(self.txs) < self.capacity:
            transaction_pool.txs.sort(key=lambda x: x.transaction_fees, reverse=True)
            for i in range(0, self.capacity):
                self.txs.append(transaction_pool.txs[i])

    # strategy two: packaging blocks by choosing txs randomly
    def choose_txs_randomly(self, transaction_pool):
        if len(self.txs) < self.capacity:
            for i in range(0, self.capacity):
                k = random.randint(0, transaction_pool.capacity-1)
                self.txs.append(transaction_pool.txs[k])

    # strategy three: package blocks by mapping the reversed six characters
    # need to optimize, a lot of time with two totally recycling
    def choose_txs_with_mapping(self, transaction_pool, miner_address):
        if len(self.txs) < self.capacity:
            address_hex = miner_address[-1:]
            capacity_bar = 0
            for tx in transaction_pool.txs:
                if (tx.hash[-1:] == address_hex) and (capacity_bar <= self.capacity):
                    capacity_bar = capacity_bar + 1
                    self.txs.append(tx)


# simulation of getting miners' address
def get_address(miner_numbers):
    miner_addresses = []
    for i in range(0, miner_numbers):
        p, s = rsa.newkeys(2048)
        pub_key = p.save_pkcs1()
        miner_address = hashlib.sha256(pub_key).hexdigest()
        miner_addresses.append(miner_address)
    return miner_addresses


# simulation of network consensus
def network_simulation(block_number):
    # block interval of different concurrency round
    timestamp_interval = []
    # concurrency blocks' sum time interval
    sum_time = 30
    # package blocks simulation: random to solve the puzzle
    for i in range(0, block_number):
        timestamp = random.randint(0, sum_time)
        timestamp_interval.append(timestamp)
    return timestamp_interval


# simulation of package strategy1
def package_simulation_strategy1(transaction_pool, fork_coefficient):
    # Initializing the network consensus
    blocks_timestamp_interval = network_simulation(fork_coefficient)
    blocks_timestamp_interval.sort()
    concurrency_block_number = fork_coefficient

    # strategy1
    blocks_with_strategy1 = []
    for i in range(0, concurrency_block_number):
        block = Block()
        block.number = i + 1
        block.time = blocks_timestamp_interval[i]

        # when the interval of two blocks producing is smaller than the time of transmitting needed
        if i - 1 < 0:
            block.choose_txs_with_transaction_fees(transaction_pool)
            k = i
        else:
            for j in range(k, i):
                if block.time - blocks_with_strategy1[j].time >= 2:
                    transaction_pool.delete_transactions(blocks_with_strategy1[j].txs)
                    block.choose_txs_with_transaction_fees(transaction_pool)
                    k = j + 1
                else:
                    block.choose_txs_with_transaction_fees(transaction_pool)
        blocks_with_strategy1.append(block)
    return blocks_with_strategy1


# simulation of package strategy2
def package_simulation_strategy2(transaction_pool, fork_coefficient):
    # Initializing the network consensus
    blocks_timestamp_interval = network_simulation(fork_coefficient)
    blocks_timestamp_interval.sort()
    concurrency_block_number = fork_coefficient

    # strategy2
    blocks_with_strategy2 = []
    for i in range(0, concurrency_block_number):
        block = Block()
        block.number = i + 1
        block.time = blocks_timestamp_interval[i]

        # when the interval of two blocks producing is smaller than the time of transmitting needed
        if i - 1 < 0:
            block.choose_txs_randomly(transaction_pool)
            k = i
        else:
            for j in range(k, i):
                if block.time - blocks_with_strategy2[j].time >= 2:
                    transaction_pool.delete_transactions(blocks_with_strategy2[j].txs)
                    block.choose_txs_randomly(transaction_pool)
                    k = j + 1
                else:
                    block.choose_txs_randomly(transaction_pool)
        blocks_with_strategy2.append(block)
    return blocks_with_strategy2


# simulation of package strategy3
def package_simulation_strategy3(transaction_pool, fork_coefficient):
    # Initializing the network consensus
    blocks_timestamp_interval = network_simulation(fork_coefficient)
    blocks_timestamp_interval.sort()
    concurrency_block_number = fork_coefficient

    # strategy3
    blocks_with_strategy3 = []
    miner_address = get_address(6)
    for i in range(0, concurrency_block_number):
        block = Block()
        block.number = i + 1
        block.time = blocks_timestamp_interval[i]
        block.miner = miner_address[i % 6]

        # when the interval of two blocks producing is smaller than the time of transmitting needed
        if i - 1 < 0:
            block.choose_txs_with_mapping(transaction_pool, block.miner)
            k = i
        else:
            for j in range(k, i):
                if block.time - blocks_with_strategy3[j].time >= 2:
                    transaction_pool.delete_transactions(blocks_with_strategy3[j].txs)
                    block.choose_txs_with_mapping(transaction_pool, block.miner)
                    k = j + 1
                else:
                    block.choose_txs_with_mapping(transaction_pool, block.miner)
        blocks_with_strategy3.append(block)
    return blocks_with_strategy3
