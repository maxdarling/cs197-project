import random 
import sys 

def dense_data(n):
    '''
    Makes dense data. Parameter 'n' is the last number, generates data [1, N].
    Everything written to 'dense.txt'
    '''
    with open("dense.txt","w") as f:
        for i in range(1, n+1):
            f.write(str(i) + "\n")

def sparse_data(n):
    '''
    Makes sparse data. Parameter 'n' is the end range. (Inclusive)
    For complete data set, n should be 2^64 - 1.
    '''
    with open("sparse.txt", "w") as f:
        for i in range(1, n+1):
            number = random.randrange(1, n)
            f.write(str(number) + "\n")

def main():
    args = sys.argv[1:]
    data_type = args[0]
    n = int(args[1])
    
    if data_type == '-dense':
        return dense_data(n)
    if data_type == '-sparse':
        return sparse_data(n)
    else:
        raise ValueError("Data Type is not valid!")    

if __name__ == '__main__':
    main()


    