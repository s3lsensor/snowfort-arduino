function dataArray = importfile(filename, startRow, endRow)
%IMPORTFILE Import numeric data from a text file as column vectors.
%   [LOCALTIME,IP,SENSORTIME,ACC_X,ACC_Y,ACC_Z,TEMPERATURE,GYRO_X,GYRO_Y,GYRO_Z]
%   = IMPORTFILE(FILENAME) Reads data from text file FILENAME for the
%   default selection.
%
%   [LOCALTIME,IP,SENSORTIME,ACC_X,ACC_Y,ACC_Z,TEMPERATURE,GYRO_X,GYRO_Y,GYRO_Z]
%   = IMPORTFILE(FILENAME, STARTROW, ENDROW) Reads data from rows STARTROW
%   through ENDROW of text file FILENAME.
%
% Example:
%   [LocalTime,IP,SensorTime,acc_x,acc_y,acc_z,temperature,gyro_x,gyro_y,gyro_z] = importfile('log2.csv',1, 25036);
%
%    See also TEXTSCAN.

% Auto-generated by MATLAB on 2016/09/19 14:01:25

%% Initialize variables.
delimiter = ',';
if nargin<=2
    startRow = 1;
    endRow = inf;
end

%% Format string for each line of text:
%   column1: text (%s)
%	column2: text (%s)
%   column3: double (%f)
%	column4: double (%f)
%   column5: double (%f)
%	column6: double (%f)
%   column7: double (%f)
%	column8: double (%f)
%   column9: double (%f)
%	column10: double (%f)
% For more information, see the TEXTSCAN documentation.
formatSpec = '%s%s%f%f%f%f%f%f%f%f%[^\n\r]';

%% Open the text file.
fileID = fopen(filename,'r');

%% Read columns of data according to format string.
% This call is based on the structure of the file used to generate this
% code. If an error occurs for a different file, try regenerating the code
% from the Import Tool.
dataArray = textscan(fileID, formatSpec, endRow(1)-startRow(1)+1, 'Delimiter', delimiter, 'EmptyValue' ,NaN,'HeaderLines', startRow(1)-1, 'ReturnOnError', false);
for block=2:length(startRow)
    frewind(fileID);
    dataArrayBlock = textscan(fileID, formatSpec, endRow(block)-startRow(block)+1, 'Delimiter', delimiter, 'EmptyValue' ,NaN,'HeaderLines', startRow(block)-1, 'ReturnOnError', false);
    for col=1:length(dataArray)
        dataArray{col} = [dataArray{col};dataArrayBlock{col}];
    end
end

%% Close the text file.
fclose(fileID);

%% Post processing for unimportable data.
% No unimportable data rules were applied during the import, so no post
% processing code is included. To generate code which works for
% unimportable data, select unimportable cells in a file and regenerate the
% script.

%% Allocate imported array to column variable names
% LocalTime = dataArray{:, 1};
% IP = dataArray{:, 2};
% SensorTime = dataArray{:, 3};
% acc_x = dataArray{:, 4};
% acc_y = dataArray{:, 5};
% acc_z = dataArray{:, 6};
% temperature = dataArray{:, 7};
% gyro_x = dataArray{:, 8};
% gyro_y = dataArray{:, 9};
% gyro_z = dataArray{:, 10};



