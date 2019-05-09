from __future__ import division
import math
import cmath
import random

from projectq import MainEngine
from projectq.ops import H, Z, All, Ry, Swap, Measure, X, get_inverse, QFT, Tensor, BasicGate
from projectq.meta import Loop, Compute, Uncompute, Control
from projectq.libs.math import AddConstant, SubConstant
import numpy as np
from scipy import linalg as lg
 
from mpi4py import MPI  

def dec2bin(num):
    l = []
    if num < 0:
        return '-' + dec2bin(abs(num))
    while True:
        num, remainder = divmod(num, 2)
        l.append(str(remainder))
        if num == 0:
            return(''.join(l[::-1]))
               
#Conrol Rotation    
def Control_Rotation(eng,Qubits,dataset,n):

    for k in range(2**n):    
        state=''
        x=k
        for j in range(n):
            if 2**(n-j-1)>x:
                state=state+'0'
            else:
                state=state+'1'
                x-=2**(n-j-1)
        k1=0
        for i in range(n):
            if state[n-i-1]=='0':
                X | Qubits[n-i-1]
            else:
                k1=k1+2**i
        with Control(eng,Qubits[0:n]):
            theta=2*math.asin(1/2**10/dataset[k1])
            Ry(theta) | Qubits[n]
        for i in range(n):
            if state[n-i-1]=='0':
                X | Qubits[n-i-1]
                
# #the function to creat the Toeplitz Matrix
def func_k(k):
    return 1/k**3

#to get the f(2*pi*i*1j/N),e.g. the eigenvalue
def func(index,n):
    val=0
    for k in range(0,2**n):
        if k!=0:
            val=val+func_k(k)*cmath.exp(2*math.pi*(1j)*index*k/2**n)
            val=val+func_k(k)*cmath.exp(-2*math.pi*(1j)*index*k/2**n)
        else:
            val=val+1
    return val
  
#Store the information of the quantum state into a classic numpy    
def qutoclass(eng,bqubits,n,n0):
    
    States0=np.zeros(2**n)*(0+0j)
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
       
        States0[i]=eng.backend.get_amplitude(StrState+str(n0),bqubits)
    return States0

def randb(eng,n,Qubits):  
    for i in range(n):
        #random.seed(i*8)
        Ry(-1*random.randint(1,100)/23) | Qubits[i]
# =============================================================================

 
if __name__ == "__main__":
    
     #Mainengine
    eng = MainEngine()
    
    #n: the dimension of the Toeplitz matrix is 2**n
    n=9
    
    Qubits=eng.allocate_qureg(n+1)
    #created a random b; the algorithm is to solve Ax=b.         
    randb(eng,n,Qubits) 
    
    b=qutoclass(eng,Qubits,n,0)
        
    #eigenvalues
    dataset1=[0]*2**n   
    for i in range(2**n):
        dataset1[i]=func(i,n)  
    dataset=[x.real for x in dataset1]
    
    '''
        Quantum
    '''
    #the QFT misses a swap operation
    for i in range(int(n/2)):
         Swap | (Qubits[i],Qubits[n-i-1])
    QFT | Qubits[0:n]
     
    #Control rotation
    Control_Rotation(eng,Qubits,dataset,n)
    
    #inverse QFT
    get_inverse(QFT) | Qubits[0:n]
    for i in range(int(n/2)):
         Swap | (Qubits[i],Qubits[n-i-1]) 
         
    #Get the quantum output and store in classical numpy. Named Quantum     
    qstate=qutoclass(eng,Qubits, n, 1)
    #nomalized
    qsum=math.sqrt(sum(abs(x)**2 for x in qstate))
    qstate=qstate/qsum
    All(Measure) | Qubits    
    '''
        Classical
    '''
    #classical method to get the circulent matrix, and solve the problem in classical, named Circulent
    F=np.zeros((2**n,2**n))*(0+0j)
    for i in range(2**n):
        for j in range(2**n):
            F[i,j]=(1/math.sqrt(2**n))*cmath.exp(-2*math.pi*1j*j*i/2**n)    
    FT=np.matrix(F).H
    landa=np.zeros((2**n,2**n))*(0+0j)
    for i in range(2**n):
        landa[i,i]=func(i,n)
    Matrix1=(FT*landa*F).real
 
    cstate1=lg.solve(Matrix1,b).real
    cnorm1=math.sqrt(sum(x**2 for x in cstate1))
    cstate1=(cstate1/cnorm1)
    
    #classical method to get the Toeplitz matrix, and solve the problem in classical, named Toeplitz
    Matrix=np.zeros((2**n,2**n))
    for i in range(2**n):
        if i!=0:
            for j in range(0,i):
                Matrix[j,i]=1/(i-j)**3
    Matrix=Matrix+Matrix.T
    Matrix=np.eye(2**n)+Matrix    
    cstate=lg.solve(Matrix,b).real
    
    cnorm=math.sqrt(sum(x**2 for x in cstate))
    cstate=cstate/cnorm
    '''
        Show the result
    '''
    print("=======================================================================")
    print("= This is the Asymptotic Quantum Algorithm for the Toeplitz Systems demo")
    print("= The algorithm solve the linear equations Ax=b, where A is a kind of Toeplitz matrix")
    print("= Change the dimension of the system by modifying the dimension variable")
    # search for the index of the max element
    print("= The dimension is: {}".format(2**n))
    print("= The distance between quantum output and classical output: ")
    qstate=qstate.real
    print(qstate-cstate)
    print("=======================================================================")
    x=range(0,2**n)
    #compare Toeplitz and the Quantum
    #plt.scatter(x,qstate)
    #plt.scatter(x,cstate)
    #plt.show()
# =============================================================================
#     #compare the two classical output, the Circulent and the Toeplitz
#     plt.scatter(x,cstate)
#     plt.scatter(x,cstate1)  
#     plt.show()
#     #compare the Circulent and the Quantum 
#     plt.scatter(x,qstate)
#     plt.scatter(x,cstate1)    
#     plt.show()
# =============================================================================
