import os
import json
import networkx as nx
import matplotlib.pyplot as plt


#  combination of document
# def combination():
#     Raw_data_path = './Raw_data_try'
#     filenames = os.listdir(Raw_data_path)
#
#     f = open('./Filter_Data/combination_results_try.txt', 'w')
#     for filename in filenames:
#         filepath = Raw_data_path + '/' + filename
#         for line in open(filepath):
#             f.writelines(line)
#             f.write('\n')
#     f.close()


# extraction of data
def extraction():
    Raw_data_path = './Raw_data'
    filenames = os.listdir(Raw_data_path)

    f = open('./Filter_Data/filter_results_try.txt', 'w')
    has_coinbase = 0
    for filename in filenames:
        filepath = Raw_data_path + '/' + filename
        for line in open(filepath):
            for tx in json.loads(line)['tx']:
                for pre_out in tx['vin']:
                    # print(pre_out)
                    if 'coinbase' in pre_out.keys():
                        has_coinbase = 1
                        break
                if has_coinbase:
                    has_coinbase = 0
                    continue
                # print(tx)
                # getting a document in which a line is a tx
                f.writelines(json.dumps(tx))
                f.write("\n")
    f.close()


# relations in DAG
def tx_DAG():
    G = nx.DiGraph()
    filepath = './Filter_Data/filter_results_try.txt'
    for line in open(filepath):
        # every node represents a single tx
        G.add_node(json.loads(line)['txid'])

    for line in open(filepath):
        for utxo in json.loads(line)['vin']:
            if utxo['pre_out'] in G.nodes:
                G.add_edge(json.loads(line)['txid'], utxo['pre_out'])
    nx.draw(G)
    print(G.nodes)
    print(G.edges)
    plt.show()


if __name__ == '__main__':
    extraction()
    tx_DAG()
