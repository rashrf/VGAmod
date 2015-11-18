width = 1680;
wtotal=2240;
height =1050;
htotal=1060;

rate = 146.2e6;
fs = 2e6;
fc = 0 ; %2*rate/wtotal;
Ns = 1000;
CarrierSignalShift = 0;
OutputSignalShift = 16;
InputSignalShift = 0;
ColorShift = 0;
  
t = [0:1/fs:(Ns-1)/fs];

fsig = 2*rate/(wtotal);
%x = (cos(2*pi*fsig*t)*100+200)*.5;
x = (cos(2*pi*fsig*t)*100)*2;
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
