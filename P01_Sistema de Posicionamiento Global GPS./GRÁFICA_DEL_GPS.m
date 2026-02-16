clc; % limpia la ventana de comandos
clear; %limpia las variables en el workspace
close all; % cierra las gráficas abiertas 


[file,path] = uigetfile("*.mat","Selecciona el archivo .mat");
if isequal(file,0), return; end
S = load(fullfile(path,file)); % carga el archivo de matlab 

P = S.Position; % Es un apuntador que apunta hacia la variable position 

% Detectar lat/lon dentro de Position (tabla/timetable o struct)
if istable(P) || istimetable(P)
    lat = pickVar(P, ["lat","latitude","Latitude","LAT","Lat"]);
    lon = pickVar(P, ["lon","longitude","Longitude","LON","Lon"]); % selecciona latitud y longitud de la tabla 
elseif isstruct(P)
    lat = pickField(P, ["lat","latitude","Latitude","LAT","Lat"]);
    lon = pickField(P, ["lon","longitude","Longitude","LON","Lon"]); % selecciona latitud y longitud en isstruct  
else
    error("Position no es table/timetable/struct. Revisa el contenido de Position.");
end

if isempty(lat) || isempty(lon)
    error("No encontré lat/lon dentro de Position. Revisa nombres de columnas/campos."); %mensaje de error 
end

%
lat = double(lat(:)); lon = double(lon(:)); %convierte a un vector de 1 
ok = isfinite(lat) & isfinite(lon); lat = lat(ok); lon = lon(ok);% detecta valores válidos de longitud y latitud 
figure('Name','Trayectoria GPS'); % abre la grágica llamada trayectoria GPS
geobasemap satellite % genera una perspectiva de satelite, se puede cambiar a streets 
hold on %permite dibujar varias cosas encima.

geoplot(lat, lon, '-');                   % línea
geoscatter(lat, lon, 6, 'filled');        % puntos
geoscatter(lat(1), lon(1), 40, 'filled'); % inicio (amarillo)
geoscatter(lat(end), lon(end), 60, 'filled'); % fin (color morado)
title("Trayectoria GPS (inicio → fin)");
geolimits([min(lat) max(lat)], [min(lon) max(lon)]);


function out = pickVar(T, opts) % busca latitud y longitud en la tabla en caso de que no se llamen asi 
out = []; n = string(T.Properties.VariableNames);% obtiene los nombres de las columnas 
for k=1:numel(opts)
    idx = find(lower(n)==lower(opts(k)),1); % busca coincidencias ignorando mayúculas y minúsculas 
    if ~isempty(idx), out = T.(n(idx)); return; end %devuelve la columna encontrada
end
end
% hace lo mismo que el bloque anterior pero en estructura struct 
function out = pickField(S, opts)
out = []; f = string(fieldnames(S));
for k=1:numel(opts)
    idx = find(lower(f)==lower(opts(k)),1);
    if ~isempty(idx), out = S.(f(idx)); return; end
end
end
