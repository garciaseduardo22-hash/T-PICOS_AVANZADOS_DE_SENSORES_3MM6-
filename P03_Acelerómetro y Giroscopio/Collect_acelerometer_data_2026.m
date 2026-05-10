%clear all;
clc;
close all;

ts=0.005;
t=0:0.1:10;

maxValue=1023;
minValue=0;

datos=zeros(length(t),2);
datosN=zeros(length(t),2);
arduino=serialport("COM4",115200);

pause(4);

disp('Recolectando Datos....')
for k=1:length(t)
    tic;
    flush(arduino);
    data = strtrim(readline(arduino));
    valores = str2double(split(data,","));
    if length(valores)==2
        datos(k,:) = valores;
        datosN(k,:) = 2*(valores' - minValue) ./ (maxValue - minValue) - 1;
    end
  while toc<ts
  end
  tiempo(k)=toc;
end
disp('Datos Recolectados...')
delete(arduino)
value1 = datos(:,1);
value2 = datos(:,2);

valueN1 = datosN(:,1);
valueN2 = datosN(:,2);

subplot(221)
plot(t,value1);
title('Ángulo pitch real');

subplot(222)
plot(t,valueN1);
title('Ángulo pitch normalizado');

subplot(223)
plot(t,value2);
title('Ángulo roll real');

subplot(224)
plot(t,valueN2);
title('Ángulo roll normalizado');

%% P1=[valueN1';valueN2']; Guardar cada clase
%Codificación datos, las dimensiones deben ser uniformes
% P = [P1 P2 P3 P4];
% 
% T1 = [-1*ones(1,101),-1*ones(1,101)];
% T2 = [-1*ones(1,101),1*ones(1,101)];
% T3 = [1*ones(1,101),-1*ones(1,101)];
% T4 = [1*ones(1,101),1*ones(1,101)];
% T = [T1 T2 T3 T4];