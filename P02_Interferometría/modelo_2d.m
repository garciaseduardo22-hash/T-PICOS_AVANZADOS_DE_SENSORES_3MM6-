clc                    
clear                   
close all    

data = readmatrix('coordenadas.xlsx');% Lee el excel, no use load porque solo jala en .csv

x = data(:,1);% lee la columna 1 del excel.
dist = data(:,2);% lee la columna 2 del excel.


dist_base = max(dist);% usa la distancia mas lejana como base(la mesa)


h = dist_base - dist; % diferencia entre la base y el objeto. 

h = movmean(h, 3);% hace un promedio de 3 puntos para suavizar 


h(h < 0) = 0;% Si por ruido aparecen alturas negativas, las convierte en cero


pasoX = 5;% Distancia entre mediciones en el eje X (mm)
altoMax = ceil(max(h));% Altura máxima del objeto redondeada hacia arriba
anchoImg = length(x) * pasoX;% Calcula el ancho total del objeto en la imagen


img = zeros(altoMax + 1, anchoImg);  % Crea una matriz llena de ceros que representará la imagen 2D


for i = 1:length(h)% Recorre cada medición tomada

    x_ini = (i-1)*pasoX + 1;% Calcula la posición inicial de la columna en la matriz
    x_fin = i*pasoX;% Calcula la posición final de la columna

    altura_col = round(h(i)); % Redondea la altura medida para usarla como índice entero

    if altura_col > 0 % Verifica que la altura sea mayor a cero
        img(1:altura_col, x_ini:x_fin) = 1;  % Llena con unos desde la base hasta la altura detectada
    end
end


img_plot = flipud(img);% Invierte la matriz verticalmente para que la base quede abajo


figure% Crea una ventana o grafica 
imagesc([0 anchoImg], [0 altoMax], img_plot)% Muestra la matriz como imagen 

axis equal % Mantiene la misma escala en ambos ejes
axis tight% Ajusta los límites del gráfico al tamaño de los datos
colormap(gray)% Usa una escala de grises para visualizar la imagen
grid on% Activa la cuadrícula

xlabel('Posición X (mm)')% Etiqueta del eje X
ylabel('Altura Y (mm)')% Etiqueta del eje Y
title('Figura 2D')  % Título de la gráfica
