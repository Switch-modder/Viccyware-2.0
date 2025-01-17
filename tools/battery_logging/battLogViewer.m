% For viewing logs generated by BehaviorDevBatteryLogging
% Author: Kevin Yoon

dirName = '/Users/kevin/src/cozmo/victor/batt_logs/csv/B23';
fileFilter = '.csv';

dirContents = dir(dirName);
files = {};
for i=1:length(dirContents)
    fileName = dirContents(i).name;
    if contains(fileName, fileFilter)
        files = [files, fileName];
    end
end


dataMap = containers.Map();
figure(1); 
for i=1:length(files)
    fileName = char(files(i));
    fullPath = [dirName, '/', fileName];
    data = csvread(fullPath, 1,0);
    dataMap(fileName) = data;
    
    x = data(:,1) / 60000; % convert ms to minutes
    y = data(:,3);        % filtered battery voltage
      
    fileName_escaped = strrep(fileName,'_','\_');
    plot(x,y, 'DisplayName', fileName_escaped);
    hold on;
end

% Draw a line
lowBatt_level = 3.6;
lineX = [0,60];
lowBatt_lineY = [lowBatt_level, lowBatt_level];
line(lineX, lowBatt_lineY, 'DisplayName', 'lowBatt');

highBatt_level = 4.1;
highBatt_lineY = [highBatt_level, highBatt_level];
line(lineX, highBatt_lineY, 'DisplayName', 'highBatt');

hold off;

title('Battery Discharge');
xlabel('Minutes');
ylabel('Volts');
legend show;
grid on;


