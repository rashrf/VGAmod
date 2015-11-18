
rate = 20e6;
fs = 1.5e6;
fc = 0;%10e6;
Ns = 50;
CarrierSignalShift = 0;
OutputSignalShift = 16;
InputSignalShift = 0;
width = 40;
height =10;
wtotal=width+10;
htotal=height;
  scale = 10;
  
t = [0:1/fs:(Ns-1)/fs];

fsig = 100e3;
x = (cos(2*pi*fsig*t)*100+200);
%x= ones(size(x))*128;
xIQ = kron(x,[1 0]); % I/Q complex
%xIQ=reshape(10*[ones(size(x));zeros(size(x))],1,2*length(x));
%xIQ=reshape([[1:length(x)]/10;zeros(size(x))],1,2*length(x));

cmd =sprintf('./testmod -r %d -c %d -s %d -O %d -w %d -W %d -h %d -H %d -o modout.wav',rate,fc,fs,OutputSignalShift,width,wtotal,height,htotal);
fid = popen(cmd, "w")
count = fwrite (fid, xIQ, 'float')
fclose(fid)

fid = fopen('modout.wav','r'); 
y = fread(fid,10000,'int');
fclose(fid)

figure(1)
plot(x)
figure(3)
plot(y)
