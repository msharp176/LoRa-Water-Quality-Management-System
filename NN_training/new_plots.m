Temperature = [];
pH = [];
Turbidity = [];

for i = 1:169
   inter = data{i};
   Temperature = vertcat(Temperature,inter(:,1));
   pH = vertcat(pH, inter(:,2));
   Turbidity = vertcat(Turbidity, inter(:,3));
end

Y = [Temperature, pH, Turbidity];

Ylabels = ["Temperature (" + char(176) + "C)","pH","Turbidity (NTUs)"];
stackedplot(Y,"Title","Water Quality Data","DisplayLabels",Ylabels, "LineWidth", 1.5)