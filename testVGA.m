vsyncOffsetSignal = -10;
vsyncOffsetCarrier = 0;
width = 1680;
wtotal=2240;
height =1050;
htotal=1089;

rate = 146.2e6;
fs = rate/64; %nicer signal to visualize at nice fractions of the rate; try 2e6 to see difference


Ns = 1000000;
CarrierSignalShift = 0;
OutputSignalShift = 16;
InputSignalShift = 0;
ColorShift = 0;
fsig = 8*rate/(wtotal);
 
t = [0:1/fs:(Ns-1)/fs];




%test carrier
fc = 64*rate/wtotal;
x= ones(size(t))*128;
xIQ = kron(x,[1 0]); % I/Q complex
cmd =sprintf('./VGAmod -r %d -c %d -s %d -O %d -w %d -W %d -h %d -H %d -L %d -v %d -V %d',rate,fc,fs,OutputSignalShift,width,wtotal,height,htotal,ColorShift,vsyncOffsetSignal,vsyncOffsetCarrier)
fid = popen(cmd, "w")
count = fwrite (fid, xIQ, 'float')
fclose(fid)


%test modulating siganl
fc=0;
x = (cos(2*pi*fsig*t)*100+200)*.5;
xIQ = kron(x,[1 0]); % I/Q complex
cmd =sprintf('./VGAmod -r %d -c %d -s %d -O %d -w %d -W %d -h %d -H %d -L %d -v %d -V %d',rate,fc,fs,OutputSignalShift,width,wtotal,height,htotal,ColorShift,vsyncOffsetSignal,vsyncOffsetCarrier)
fid = popen(cmd, "w")
count = fwrite (fid, xIQ, 'float')
fclose(fid)

%test modulationfc = 64*rate/wtotal;
x = (cos(2*pi*fsig*t)*100+200)*.5;
fc = 64*rate/wtotal;
xIQ = kron(x,[1 0]); % I/Q complex
cmd =sprintf('./VGAmod -r %d -c %d -s %d -O %d -w %d -W %d -h %d -H %d -L %d -v %d -V %d',rate,fc,fs,OutputSignalShift,width,wtotal,height,htotal,ColorShift,vsyncOffsetSignal,vsyncOffsetCarrier)
fid = popen(cmd, "w")
count = fwrite (fid, xIQ, 'float')
fclose(fid)
