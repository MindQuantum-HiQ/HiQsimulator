from hiq.projectq.backends import StabilizerSimulator

def run_qecc9():
	
    #init simulator
    simulator = StabilizerSimulator()
    
    #allocate
    simulator.allocate_qureg(12)
    
    #start
    print("= Encoded the qubit, state is: |0>")
    #circuit
    simulator.cnot(0, 3)
    simulator.cnot(3, 0)

    simulator.cnot(3, 6)
    simulator.cnot(3, 9)
    simulator.h(3)
    simulator.cnot(3, 4)
    simulator.cnot(3, 5)
    simulator.h(6)
    simulator.cnot(6, 7)
    simulator.cnot(6, 8)
    simulator.h(9)
    simulator.cnot(9, 10)
    simulator.cnot(9, 11)

    simulator.h(3)
    simulator.h(4)
    simulator.h(5)
    simulator.h(6)
    simulator.h(7)
    simulator.h(8)
    simulator.h(9)
    simulator.h(10)
    simulator.h(11)
    simulator.cnot(1, 3)
    simulator.cnot(1, 4)
    simulator.cnot(1, 5)
    simulator.cnot(1, 6)
    simulator.cnot(1, 7)
    simulator.cnot(1, 8)
    simulator.cnot(1, 9)
    simulator.cnot(1, 10)
    simulator.cnot(1, 11)
    simulator.h(3)
    simulator.h(4)
    simulator.h(5)
    simulator.h(6)
    simulator.h(7)
    simulator.h(8)
    simulator.h(9)
    simulator.h(10)
    simulator.h(11)

    simulator.cnot(2, 3)
    simulator.cnot(2, 4)
    simulator.cnot(2, 5)
    simulator.cnot(2, 6)
    simulator.cnot(2, 7)
    simulator.cnot(2, 8)
    simulator.cnot(2, 9)
    simulator.cnot(2, 10)
    simulator.cnot(2, 11)

    simulator.cnot(9, 11)
    simulator.cnot(9, 10)
    simulator.h(9)
    simulator.cnot(6, 8)
    simulator.cnot(6, 7)
    simulator.h(6)
    simulator.cnot(3, 5)
    simulator.cnot(3, 4)
    simulator.h(3)
    simulator.cnot(3, 9)
    simulator.cnot(3, 6)
    print("= Decoded the qubit, state is: |{}>".format(simulator.measure(3)))
    #sync
    simulator.sync()
	
if __name__ == "__main__":

    print("==================================================================")
    print("= This is the Quantum Error-Correcting Codes 9 demo")
    print("= Encodes a qubit of your choice using the Shor 9-qubit quantum error-correcting code")
    print("= Applies an encoded Pauli operation of your choice to the codeword")
    print("Decodes the codeword and measures the result")
    run_qecc9()
    print("==================================================================")
    
