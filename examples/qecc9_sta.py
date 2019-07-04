from projectq.ops import CNOT, H, Measure
from hiq.projectq.backends import StabilizerSimulator
from hiq.projectq.cengines import HiQMainEngine

def run_qecc9():
	
    #init simulator
    simulator = StabilizerSimulator(12)
    eng = HiQMainEngine(simulator, [])
    
    #allocate
    qubits = eng.allocate_qureg(12)
    
    #start
    print("= Encoded the qubit, state is: |0>")
    #circuit
    CNOT | (qubits[0], qubits[3])
    CNOT | (qubits[3], qubits[0])

    CNOT | (qubits[3], qubits[6])
    CNOT | (qubits[3], qubits[9])
    H | qubits[3]
    CNOT | (qubits[3], qubits[4])
    CNOT | (qubits[3], qubits[5])
    H | qubits[6]
    CNOT | (qubits[6], qubits[7])
    CNOT | (qubits[6], qubits[8])
    H | qubits[9]
    CNOT | (qubits[9], qubits[10])
    CNOT | (qubits[9], qubits[11])

    H | qubits[3]
    H | qubits[4]
    H | qubits[5]
    H | qubits[6]
    H | qubits[7]
    H | qubits[8]
    H | qubits[9]
    H | qubits[10]
    H | qubits[11]
    CNOT | (qubits[1], qubits[3])
    CNOT | (qubits[1], qubits[4])
    CNOT | (qubits[1], qubits[5])
    CNOT | (qubits[1], qubits[6])
    CNOT | (qubits[1], qubits[7])
    CNOT | (qubits[1], qubits[8])
    CNOT | (qubits[1], qubits[9])
    CNOT | (qubits[1], qubits[10])
    CNOT | (qubits[1], qubits[11])
    H | qubits[3]
    H | qubits[4]
    H | qubits[5]
    H | qubits[6]
    H | qubits[7]
    H | qubits[8]
    H | qubits[9]
    H | qubits[10]
    H | qubits[11]

    CNOT | (qubits[2], qubits[3])
    CNOT | (qubits[2], qubits[4])
    CNOT | (qubits[2], qubits[5])
    CNOT | (qubits[2], qubits[6])
    CNOT | (qubits[2], qubits[7])
    CNOT | (qubits[2], qubits[8])
    CNOT | (qubits[2], qubits[9])
    CNOT | (qubits[2], qubits[10])
    CNOT | (qubits[2], qubits[11])

    CNOT | (qubits[9], qubits[11])
    CNOT | (qubits[9], qubits[10])
    H | qubits[9]
    CNOT | (qubits[6], qubits[8])
    CNOT | (qubits[6], qubits[7])
    H | qubits[6]
    CNOT | (qubits[3], qubits[5])
    CNOT | (qubits[3], qubits[4])
    H | qubits[3]
    CNOT | (qubits[3], qubits[9])
    CNOT | (qubits[3], qubits[6])
    
    Measure | qubits[3]
    #flush      
    eng.flush() 
    
    print("= Decoded the qubit, state is: |{}>".format(int(qubits[3])))


	
if __name__ == "__main__":

    print("==================================================================")
    print("= This is the Quantum Error-Correcting Codes 9 demo")
    print("= Encodes a qubit of your choice using the Shor 9-qubit quantum error-correcting code")
    print("= Applies an encoded Pauli operation of your choice to the codeword")
    print("Decodes the codeword and measures the result")
    run_qecc9()
    print("==================================================================")
    
