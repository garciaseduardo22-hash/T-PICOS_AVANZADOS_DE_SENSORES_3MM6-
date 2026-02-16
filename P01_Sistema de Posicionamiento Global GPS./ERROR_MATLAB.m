clc; % limpia la ventana de comandos
clear; %limpia las variables en el workspace
close all; % cierra las gráficas abiertas 


[file,path] = uigetfile("*.mat","Selecciona el archivo .mat");
if isequal(file,0)
    disp("No se seleccionó archivo.");
    return;
end

load(fullfile(path,file)); % todo este bloque es para cargar el archivo de matlab 

T = Position;% es timetable
T.Properties.VariableNames % ocupaba ver el nombre de las variables, (estaban en ingles), t propieties funciona en tablas no en vectores ni matrices

lat = T.latitude;% convierto la latitud de la tabla a una variable trabajable            
lon = T.longitude; % convierto la latitud de la tabla a una variable trabajable         

% punto de referencia o punto promedio
lat0 = mean(lat);   % Promedio de latitudes
lon0 = mean(lon);   % Promedio de longitudes


R = 6371000;   % Radio promedio de la Tierra en metros

% Convierte grados de latitud y longitud a radianes
phi1 = deg2rad(lat0);
lambda1 = deg2rad(lon0);

phi2 = deg2rad(lat);
lambda2 = deg2rad(lon);

% Diferencias 
dphi = phi2 - phi1;
dlambda = lambda2 - lambda1;

% Formula Haversine
a = sin(dphi/2).^2 + cos(phi1).*cos(phi2).*sin(dlambda/2).^2;

d = 2*R*asin(sqrt(a));  % Distancia en metros del error por punto


MAE  = mean(d);           % Error medio
RMSE = sqrt(mean(d.^2));  % Error cuadratico medio
MAXE = max(d);            % Error maximo


fprintf('Error Medio (MAE): %.3f m\n', MAE);% muestra el texto y el valor del error medio
fprintf('RMSE: %.3f m\n', RMSE);% muestra el texto y el valor del error cuadratico medio
fprintf('Error Maximo: %.3f m\n', MAXE);% muestra el texto y el valor del error máximo 

