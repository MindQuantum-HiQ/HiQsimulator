from __future__ import division
import numpy as np
np.seterr(divide='ignore', invalid='ignore')
from scipy import linalg
import math
import time
import random
from projectq.meta import Control,Compute,Uncompute
from projectq.ops import QubitOperator
from projectq.ops import (X,
                          Y,
                          Z,
                          CNOT,
                          Measure,
                          H,
                          R,
                          T,
                          S,
                          Sdag,
                          Ry,
                          R,
                          QFT,
                          Swap,
                          TimeEvolution,
                          get_inverse,
                          BasicMathGate)
from projectq.backends import CircuitDrawer, CommandPrinter
from projectq.cengines import (MainEngine,
                               AutoReplacer,
                               LocalOptimizer,
                               TagRemover,
                               DecompositionRuleSet)
import projectq.setups.decompositions
                               
from hiq.projectq.cengines import GreedyScheduler, HiQMainEngine
from hiq.projectq.backends import SimulatorMPI
 
from mpi4py import MPI

def para():
    # input array define, used to build a matrix
    input_array1=[-1,4]
    input_array2=[2,5,4,9]
    input_array3=[1,2,4,3,5,6.2,9,12]
    input_array4=[2,4,3,1,8,21,7,15,5,17,19,33,30,25,9,30]
    input_array5=np.zeros(2**4)
    for i in range(len(input_array5)):
        random.seed(i)
        input_array5[i]=random.randint(1,10000)/314+1
    InputArray=input_array5
    # n is the number of qubits needed to build a matrix
    MatrixLen=len(InputArray)
    n=int(np.log2(MatrixLen))
    # input matrix define
    Hadamard=np.array([[1,1],[1,-1]])/np.sqrt(2)
    HN=Hadamard
    #print ('HN Matrix is:')
    for i in range(n-1):
        HN=np.kron(HN,Hadamard)   
    '''
    for i in HN:
        print (i)
    '''
    InputMatrix=np.zeros((MatrixLen,MatrixLen))
    for i in range(2**n):
        ax=(HN[i,:].reshape(2**n,1))/np.sqrt(2**n)
        ay=ax.T
        InputMatrix+=InputArray[i]*(np.dot(ax,ay))
    '''
    for i in range(MatrixLen):
        for j in range(MatrixLen):
            if i==0:
                input_matrix[i][j]=input_array[j]
            elif j<MatrixLen-1:
                input_matrix[i][j]=input_matrix[i-1][j+1]
            else:
                input_matrix[i][j]=input_matrix[0][MatrixLen-i-1]
    for i in range(MatrixLen):
        input_matrix[i][i]=input_array[0]
    '''
    #ExpMatrix=linalg.expm(InputMatrix*-(1j))
    # input a vector to calculate
    '''
    SquareSum=0
    InputVector=np.zeros(MatrixLen)
    for i in range(MatrixLen):
        random.seed(i)
        InputVector[i]=random.randint(1,100)
        SquareSum+=InputVector[i]**2
    for i in range(MatrixLen):
        InputVector[i]/=SquareSum
    '''
    #print (ExpMatrix)
    return n,InputMatrix,InputArray

def ShortSymMatrixSolver(eng):
    n=para()[0]
    Matrix=para()[1]
    Array=para()[2]
    XNum=0
    RyNum=0
    HNum=0
    CNXNum=0
    CRyNum=0
    r=30
    Qubits=eng.allocate_qureg(n+2**n+1)
    States0=np.zeros(2**n)
    Statest=np.zeros(2**n)
    Qubits=eng.allocate_qureg(n+1)
    for i in range(n):
        random.seed(i*3)
        Ry(random.randint(1,300)/23) | Qubits[i]
        
    eng.flush()
    #Output the initial state
    for i in range(2**n):
        state=np.zeros(n)
        x=i
        for j in range(n):
            if 2**(n-j-1)>x:
                state[j]=0
            else:
                state[j]=1
                x-=2**(n-j-1)
        # Change the list of states to string format
        StrState=''
        for k in range(n):
            StrState+=str(int(state[k]))
        States0[i]=np.real(eng.backend.get_amplitude(
                            StrState+'0',Qubits))
    
    with Compute(eng):
        for i in range(n):
            H | Qubits[i]
    for i in range(2**n):
        for j in range(n):
            if i%(2**j)==0:
                X | Qubits[n-j-1]
        with Control(eng,Qubits[0:n]):
            theta=2*math.asin(1/Array[i])
            #theta=(2/Array[2**n-i-1]/max(Array))/(2**r)
            Ry(theta) | Qubits[-1]
            CRyNum+=1
    
    Uncompute(eng)
    
    eng.flush()
    
    for i in range(2**n):
        state=np.zeros(n)
        x=i
        for j in range(n):
            if 2**(n-j-1)>x:
                state[j]=0
            else:
                state[j]=1
                x-=2**(n-j-1)
        # Change the list of states to string format
        StrState=''
        for k in range(n):
            StrState+=str(int(state[k]))
        #print (state)
        #print (StrState)
        Statest[i]=np.real(eng.backend.get_amplitude(
                                        StrState+'1',Qubits))
    Measure | Qubits
    return States0,Statest

def SymMatrixSolver(eng):
    n=para()[0]
    Matrix=para()[1]
    Array=para()[2]
    XNum=0
    RyNum=0
    HNum=0
    CNXNum=0
    CRyNum=0
    r=30
    Qubits=eng.allocate_qureg(n+2**n+1)
    States0=np.zeros(2**n)
    Statest=np.zeros(2**n)
    # Initial state papreparation
    for i in range(n):
        random.seed(i*7)
        Ry(random.randint(1,300)/23) | Qubits[i]
        #Ry(2) | Qubits[i]
        RyNum+=1
    eng.flush()
    #Output the initial state
    for i in range(2**n):
        state=np.zeros(n)
        x=i
        for j in range(n):
            if 2**(n-j-1)>x:
                state[j]=0
            else:
                state[j]=1
                x-=2**(n-j-1)
        # Change the list of states to string format
        StrState=''
        for k in range(n):
            StrState+=str(int(state[k]))
        States0[i]=np.real(eng.backend.get_amplitude(
                            StrState+'0'*(2**n)+'0',Qubits))
        
    with Compute(eng):
        for i in range(n):
            H | Qubits[i]
            HNum+=2
        for j in range(2**n):
            for k in range(n):
                if j%(2**k)==0:
                    X | Qubits[n-k-1]
                    XNum+=2
            with Control(eng,Qubits[0:n]):
                X | Qubits[n+j]
                CNXNum+=2
    
    # Control Ry
    for i in range(2**n):
        with Control(eng,Qubits[n+i]):
            #theta=(1/Array[i])/(2**r)
            theta=2*math.asin(1/Array[i])
            #theta=(2/Array[2**n-i-1]/max(Array))/(2**r)
            Ry(theta) | Qubits[-1]
            CRyNum+=1
            
    Uncompute(eng)
    eng.flush()
    
    for i in range(2**n):
        state=np.zeros(n)
        x=i
        for j in range(n):
            if 2**(n-j-1)>x:
                state[j]=0
            else:
                state[j]=1
                x-=2**(n-j-1)
        # Change the list of states to string format
        StrState=''
        for k in range(n):
            StrState+=str(int(state[k]))
        #print (state)
        #print (StrState)
        Statest[i]=np.real(eng.backend.get_amplitude(
                                        StrState+'0'*(2**n)+'1',Qubits))
        
    Measure | Qubits
    print ('Number of X gates:',XNum,
           '\nNumber of H gates:',HNum,
           '\nNumber of Ry gates:',RyNum,
           '\nNumber of CNX gates:',CNXNum,
           '\nNumber of CRy gates:',CRyNum)
    return States0,Statest

def Norm(array1d):
    SquareSum=0
    for i in array1d:
        SquareSum+=i**2
    for i in range(len(array1d)):
        array1d[i]=array1d[i]/SquareSum
    return array1d

def CompareNorm(array1d):
    FinalArray=[1]
    for i in range(len(array1d)-1):
        FinalArray.append(FinalArray[0]*array1d[i+1]/array1d[0])
    return np.array(FinalArray)

if __name__ == "__main__":
    Matrix=para()[1]
    InvMatrix=np.linalg.inv(Matrix)
    # create a main compiler engine with a simulator backend:
    backend = SimulatorMPI(gate_fusion=True, num_local_qubits=22)
    # backend = CircuitDrawer()
    # locations = {}
    # for i in range(module.nqubits):
    #     locations[i] = i
    # backend.set_qubit_locations(locations)
    
    cache_depth = 10
    rule_set = DecompositionRuleSet(modules=[projectq.setups.decompositions])
    engines = [TagRemover(),
    		   LocalOptimizer(cache_depth),
    		   AutoReplacer(rule_set),
    		   TagRemover(),
    		   LocalOptimizer(cache_depth),
    		   #CommandPrinter(),
           GreedyScheduler()
    		   ]
    
    eng = HiQMainEngine(backend, engines) 
    State0,Statet=ShortSymMatrixSolver(eng)
    print ('\nThe Matrix A is:')
    for i in range(len(Matrix)):
        print (Matrix[i],'\t[',round(State0[i],3),']')
    IdealResult=np.dot(InvMatrix,State0)
    print ('Ideal Results:\t Experimental Results:')
    for i in range(len(IdealResult)):
        print (CompareNorm(IdealResult)[i],'\t',CompareNorm(Statet)[i])
    '''
    # Result Drawer Part
    draw.draw2d_2list(CompareNorm(Statet),
                      CompareNorm(np.dot(InvMatrix,State0)))
    '''
    # Circuit Drawer Part
    '''
    drawing_engine = CircuitDrawer()
    eng = MainEngine(drawing_engine)
    ShortSymMatrixSolver(eng)
    eng.flush()
    file=open('SymMatrixSolver.txt','w+')
    file.write(drawing_engine.get_latex())
    file.close()
    '''












