import serial
import cv2
import time
import numpy as np
class datareadcam:
    def __init__(self,comport1,brate,H,W):
        self.COMMAND = ['*', 'R', 'D', 'Y', '*']
        self.serialcomm = serial.Serial(comport1, brate)
        self.Hight1=H
        self.Width1=W
        self.serialcomm.bytesize = serial.EIGHTBITS
        self.serialcomm.stopbits = serial.STOPBITS_ONE
        self.serialcomm.parity = serial.PARITY_NONE
        self.serialcomm.timeout=0.4
    def opendport(self):
        if(self.serialcomm.is_open == False):
            self.serialcomm.open()
        if(self.serialcomm.is_open == False):
            print("cannot openport")
        else:
            print("openport")
    def waitfordata(self, index=0):
        if index < len(self.COMMAND):
            a = self.readdata()
            a = chr(a)
            if self.COMMAND[index] == a:
                return self.waitfordata(index + 1)
            else:
                return False
        return True
    def readdata(self):
        data = self.serialcomm.read()
        if (len(data) != 1):
            return False
        return int.from_bytes(data, "big") & 0xFF
    def mapdata(self):
        while not self.waitfordata():
            pass
        img_arr = np.zeros([self.Hight1, self.Width1, 1], dtype=np.uint8)
        for x in range(img_arr.shape[1]):
            for y in range(img_arr.shape[0]):
                temp = self.readdata()
                img_arr[y, self.Width1 - x - 1] = [temp]
        return img_arr
    def saveimg(self,img,index1):
        if img is not None:
            print(index1)
            cv2.imwrite('C:/out/' + str(index1) + '.png',img)
        else:
            print("cant save img ")
    def scanca(self,i):
        for i1 in range(7):
            c.waitfordata()
            v=c.mapdata()
            c.saveimg(v,int(i1+i))
    def readphoto2(self,e1,p,st):
        for i in range(3):
            if(e1[0] != None):
                pig="C:/out/"+str(e1[2])+".png"
                break
            else:
                cc=self.crop(str(e1[2]))
                e1=cc
        image = cv2.imread(pig, cv2.IMREAD_GRAYSCALE)
        maxx=0
        maxy=0
        minx=0
        miny=0
        if(int(e1[0][0]>e1[1][0])):
            maxy=int(e1[0][0])
            miny=int(e1[1][0])
        else:
            maxy=int(e1[1][0])
            miny=int(e1[0][0])
        if(int(e1[0][1])>int(e1[1][1])):
            maxx=int(e1[0][1])
            minx=int(e1[1][1])
        else:
            maxx=int(e1[1][1])
            minx=int(e1[0][1])
        centerY=(maxy-miny) //2 +  miny # ตัดเเกน x
        centerX=(maxx-minx)//2 + minx # ตัดเเกน y
        Unitx=(maxx-minx)//20
        Unity=(maxy-miny)//20
        Q1,Q2,Q3,Q4=[],[],[],[]
        py=[miny+4*Unity,miny+7*Unity,miny+14*Unity,miny+17*Unity]
        px=[minx+4*Unitx,minx+7*Unitx,minx+14*Unitx,minx+17*Unitx]
        for i in range(2):
            for t in range(2):
                Q1.append(int(image[py[i],px[t]]))
                Q2.append(int(image[py[i],px[t+2]]))
                Q3.append(int(image[py[i+2],px[t]]))
                Q4.append(int(image[py[i+2],px[t+2]]))
                image[py[i],px[t]]=250
                image[py[i],px[t+2]]=250
                image[py[i+2],px[t]]=250
                image[py[i+2],px[t+2]]=250
        # cv2.namedWindow('image',cv2.WINDOW_AUTOSIZE)
        # cv2.imshow('pp', image)
        # cv2.waitKey()
        Q1.append(int(Q1[0]+Q1[1]+Q1[2]+Q1[3])//4)
        Q2.append(int(Q2[0]+Q2[1]+Q2[2]+Q2[3])//4)
        Q3.append(int(Q3[0]+Q3[1]+Q3[2]+Q3[3])//4)
        Q4.append(int(Q4[0]+Q4[1]+Q4[2]+Q4[3])//4)
        print(Q1)
        print(Q2)
        print(Q3)
        print(Q4)
        s=''
        s1=""
        for i in range(len(Q1)):
            s1+=str(Q1[i])
            s1+=","
        for i in range(len(Q2)):
            s1+=str(Q2[i])
            s1+=","
        for i in range(len(Q3)):
            s1+=str(Q3[i])
            s1+=","
        for i in range(len(Q4)):
            s1+=str(Q4[i])
            s1+=","
        s1+=str("Y,")
        for i in range(len(py)):
            s1+=str(py[i])
            s1+=","
        s1+=str("X,")
        for i in range(len(px)):
            s1+=str(px[i])
            s1+=","
        # f=(Q1[-1]+Q2[-1]+Q3[-1]+Q4[-1])//4
        if(st==0):
            f=90
        else:
            f=50
        if(Q1[4]>=f):
            s+="1"
        else:
             s+="0"
        if(Q2[4]>=f):
            s+="1"
        else:
             s+="0"
        if(Q3[4]>=f):
            s+="1"
        else:
             s+="0"
        if(Q4[4]>=f):
            s+="1"
        else:
             s+="0"
        print(s)
        if(p==0):
            return s
        else:
            return s1,s
    def readPhoto1checktrue(self,name1):
        pig="C:/out/"+str(name1)+".png"
        image = cv2.imread(pig, cv2.IMREAD_GRAYSCALE)
        dataQ11=[]
        for i in range(20):
            dataQ11.append(int(image[120,i+30]))
            image[120,i+50]=250
        dataQ11.sort()
        # cv2.namedWindow('image',cv2.WINDOW_AUTOSIZE)
        # cv2.imshow('pp', image)
        # cv2.waitKey()
        if(abs(dataQ11[0]-dataQ11[-1]) > 13 ) :
            return False
        else:
            return True
    def crop(self,name1):
        if(self.readPhoto1checktrue(name1)): 
            filename = 'C:/out/'+str(name1)+'.png'
            print(filename)
            img = cv2.imread(filename)
            gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
                # find Harris corners
            gray = np.float32(gray)
            dst = cv2.cornerHarris(gray,2,3,0.04)
            dst = cv2.dilate(dst,None)
            ret, dst = cv2.threshold(dst,0.0001*dst.max(),255,0)
            dst = np.uint8(dst)
            ret, labels, stats, centroids = cv2.connectedComponentsWithStats(dst)
            criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 100, 0.0001)
            corners = cv2.cornerSubPix(gray,np.float32(centroids),(5,5),(-1,-1),criteria)
            res = np.hstack((centroids,corners))
            res = np.int0(res)
            y1=res[:,1]
            x1=res[:,0]
            v1=[]
            v2=[]
            for i in range(len(y1)):
                for b in range(len(y1)):
                    if(y1[i] <= 300 and y1[i] > 15 and abs(y1[i]-y1[b])>=195 and abs(y1[i]-y1[b])<=230):
                        if(abs(x1[i]-x1[b])>=195 and abs(x1[i]-x1[b])<= 235 ):
                            if(y1[i]<y1[b]):
                                v1.append(y1[i])
                                v1.append(x1[i])
                                v2.append(y1[b])
                                v2.append(x1[b])
                            else:
                                v1.append(y1[b])
                                v1.append(x1[b])
                                v2.append(y1[i])
                                v2.append(x1[i])
                            break
                if(len(v2) > 0):
                    break
            e1=[v1,v2,name1]
            print(v1,v2)
                # img[v1[0],v1[1]]=[0,0,255]
                # img[v2[0],v2[1]] = [0,255,0]
                # cv2.imshow('dst',img)
                # if cv2.waitKey(0) & 0xff == 27:
                #     cv2.destroyAllWindows()
        else:
            return False
        if(len(v1)==0):
            return False
        else:
            return e1
    def loopread(self,start1,stop1):
        if(stop1>start1):
            for x in range(stop1):
                v=self.readPhoto1checktrue(stop1-x)
                if(v==True):
                    return stop1-x
                    break
            else:
                return False
        else:
            for x in range( stop1,start1):
                v=self.readPhoto1checktrue(x)
                if(v==True):
                    return x
                    break
            else:
                return False
    def Superloopcheck(self,start1,stop1,mod):
        c1=[]
        if(mod ==3):
            a1=self.loopread(start1+2,stop1-10)
            a2=self.loopread(stop1-10,stop1-5)
            a3=self.loopread(stop1-5,stop1)
            c1.append(a1)
            c1.append(a2)
            c1.append(a3)
        else:
            a1=self.loopread(start1,stop1)
            c1.append(a1)
        return c1
#setting port 
c=datareadcam('COM3',1000000,320,240)
c2=datareadcam('COM6',9600,320,240)
c2.opendport()
c.opendport()
# variable
st=0
i1=["A","B","C"]
senb1=""
senb2=""
senb3=""
#star program 
while True:
    #input of aduino pc 1
    #mode 1 3 scan 
    # n1=int.from_bytes(ark1, "big") 
    # print(n1)
    ark1=c2.serialcomm.readline()
    i=ark1.decode('ascii')
    print(i)
    ark2="stop"
    vv=0
    # i = input("Enter Input: ")
    try:
        if(i=="g"):
            # c2.serialcomm.write(i1[0].encode())
            # time.sleep(0.5)
            vv=1
            print("mode 1 : 3 scan :")
            v1,v2,v3=[],[],[]
            for z in range(len(i1)):  
                c2.serialcomm.write(i1[z].encode())
                time.sleep(0.5)
                # ark1=c2.serialcomm.readline()
                # v=int(ark1.decode('ascii'))
                # print(v)
                if(z==0):
                    time.sleep(0.5)
                    c.scanca(0)
                    for i in range(7):
                        e1=c.crop(6-i)
                        if(e1 != False):
                            break
                    send1=c.readphoto2(e1,0,st)
                    
                if(z==1):
                    time.sleep(0.5)
                    c.scanca(7)
                    for i in range(7):
                        e1=c.crop(13-i)
                        if(e1 != False):
                            break
                    
                    send2=c.readphoto2(e1,0,st)

                if(z==2):
                    time.sleep(0.5)
                    c.scanca(14)
                    for i in range(7):
                        e1=c.crop(20-i)
                        if(e1 != False):
                            break
                    send3=c.readphoto2(e1,0,st)
                    
                st+=1
            c2.serialcomm.write(i1[0].encode())
            time.sleep(0.5)
            sendreal= send1+"R,"+send2+"c,"+send3+"L"
            c2.serialcomm.write(sendreal.encode())
            time.sleep(0.5)
            # c2.serialcomm.write(i1[0].encode())
            # print(sendreal)
        
        #rotation -45
        elif(i==send1):
            vv=1
            print("mode 2 : 1 scan 1 Select right")
            c2.serialcomm.write(i1[0].encode())
            time.sleep(2)
            c.scanca(0)
            for i in range(7):
                e1=c.crop(6-i)
                if(e1 != False):
                    break
            sendR,send1=c.readphoto2(e1,1,st)
            c2.serialcomm.write(sendR.encode())
            time.sleep(0.5)
            print(sendR)
        #rotation 0
        elif(i==send2):
            vv=1
            print("mode 2 : 1 scan 1 Select Center ")
            c2.serialcomm.write(i1[1].encode())
            time.sleep(2)
            c.scanca(7)
            for i in range(7):
                e1=c.crop(13-i)
                if(e1 != False):
                    break
            sendC,send2=c.readphoto2(e1,1,st)
            c2.serialcomm.write(sendC.encode())
            time.sleep(0.5)
            print(sendC)
        #rotation 45
        elif(i==send3):
            vv=1
            print("mode 2 : 1 scan 1 Select Left")
            c2.serialcomm.write(i1[2].encode())
            time.sleep(2)
            c.scanca(14)
            for i in range(7):
                e1=c.crop(20-i)
                if(e1 != False):
                    break
            sendL,send3=c.readphoto2(e1,1,st)
            c2.serialcomm.write(sendL.encode())
            time.sleep(0.5)
            print(sendL)
        # out
        if i == "3":
            print('finished')
            break
    except:
        if(vv==1):
            print(" rephoto ")
            a="rephoto"
            c2.serialcomm.write(a.encode())
c.serialcomm.close()
    

    