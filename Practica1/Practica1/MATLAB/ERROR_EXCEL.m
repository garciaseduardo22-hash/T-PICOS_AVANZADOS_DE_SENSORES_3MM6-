clc; % limpia la ventana de comandos 
clear; %limpia las variables de workspace
close all;% cierra las gráficas 

[file,path] = uigetfile("*.xlsx","Selecciona el archivo Excel");
if isequal(file,0)
    disp("No se seleccionó archivo.");
    return;
end

T = readtable(fullfile(path,file)); % carga el archivo de excel 

T.Properties.VariableNames % Ver nombres de columnas


lat = T.Latitud;% cambia el nombre de la variable en la tabla a lat 
lon = T.Longitud;% cambia el nombre de la variable en la tabla a lon


% el promedio sera tomado como mi punto de referencia 
lat0 = mean(lat); % mean calcula el promedio de todos los valores de latitud del vector 
lon0 = mean(lon); % mean calcula el promedio de todos los valores de longitud del vector

% Formula de haversine(considera la curvatura de la tierra por ende es mejor)

R = 6371000; % radio promedio de la tierra

phi1 = deg2rad(lat0);
lambda1 = deg2rad(lon0); % convierte latitud y longitud de radianes a grados

phi2 = deg2rad(lat);
lambda2 = deg2rad(lon); % convierte las medidas del GPS de radianes a grados

dphi = phi2 - phi1;
dlambda = lambda2 - lambda1; % Aquí se calcula la diferencia angular entre cada punto medido y el punto de referencia

a = sin(dphi/2).^2 + cos(phi1).*cos(phi2).*sin(dlambda/2).^2; %fórmula de Haversine

d = 2*R*asin(sqrt(a)); % calcula la distancia del error "a" en metros 


MAE  = mean(d); % error promedio del GPS
RMSE = sqrt(mean(d.^2)); % eleva el error al cuadrado, se usa en ingeniería
MAXE = max(d); % error máximo 

fprintf('Error Medio (MAE): %.3f m\n', MAE);% muestra el texto y el valor del error medio
fprintf('RMSE: %.3f m\n', RMSE);% muestra el texto y el valor del error cuadratico medio
fprintf('Error Maximo: %.3f m\n', MAXE);% muestra el texto y el valor del error máximo

