Barycentric_Lagrange()

function Barycentric_Lagrange()
    syms x % Symbolic Variable x

    Throttle_Percentages = (0:10)*10; 
    ADC_Vector = [120, 190, 230, 330, 410, 510, 613, 620, 732, 812, 855]; %Note, these are just made up for testing

    if length(Throttle_Percentages) ~= length(ADC_Vector)
        disp('Ensure both array lengths match!')
        return
    end

    w_i = [];
    Qx = sym([]);
    
    %{  
        DESMOS VERIFICATION (Done by hand...)

        \left(0,120\right),\left(10,190\right),\left(20,230\right),\left(30,330\right),\left(40,410\right),\left(50,510\right),\left(60,613\right),\left(70,620\right),\left(80,732\right),\left(90,812\right),\left(100,855\right)
        f\left(x\right)=\left(x-0\right)\left(x-10\right)\left(x-20\right)\left(x-30\right)\left(x-40\right)\left(x-50\right)\left(x-60\right)\left(x-70\right)\left(x-80\right)\left(x-90\right)\left(x-100\right)\left(\frac{\left(2.7557\cdot10^{-17}\right)}{\left(x-0\right)}181+\frac{\left(-2.7557\cdot10^{-16}\right)}{\left(x-10\right)}190+\frac{\left(1.2401\cdot10^{-15}\right)}{\left(x-20\right)}230+\frac{\left(-3.3069\cdot10^{-15}\right)}{\left(x-30\right)}330+\frac{\left(5.7870\cdot10^{-15}\right)}{\left(x-40\right)}410+\frac{\left(-6.9444\cdot10^{-15}\right)}{\left(x-50\right)}510+\frac{\left(5.7870\cdot10^{-15}\right)}{\left(x-60\right)}613+\frac{\left(-3.3069\cdot10^{-15}\right)}{\left(x-70\right)}620+\frac{\left(1.2401\cdot10^{-15}\right)}{\left(x-80\right)}732+\frac{\left(-2.7557\cdot10^{-16}\right)}{\left(x-90\right)}812+\frac{\left(2.7557\cdot10^{-17}\right)}{\left(x-100\right)}855\right)\left\{0<x<100\right\}
    
    %}

    for i = 1 : length(ADC_Vector) % MATLAB uses 1 based indexing for arrays!!
        Current = Throttle_Percentages(i);
        
        Temp_Array = [];
        Temp_Index = 1;
        
        for j = 1 : length(Throttle_Percentages)
            if(Throttle_Percentages(j) ~= Current)
                Temp_Array(Temp_Index) = Throttle_Percentages(i) - Throttle_Percentages(j);
                Temp_Index = Temp_Index + 1;
            end
        end
        w_i{i} = Temp_Array;

        disp(w_i{i});

        w_i{i} = prod(1./w_i{i});

        disp(w_i{i});
    end

    Lx = x - Throttle_Percentages;
    Lx = prod(Lx);

    fprintf('L(x): %s\n', Lx)

    for i = 1 : length(ADC_Vector)
        Qx(i) = w_i{i} ./ (x - Throttle_Percentages(i)) .* ADC_Vector(i);
    end

    Qx = sum(Qx);
    Qx = vpa(Qx);
    
    fprintf('Q(x): %s\n', Qx(1));

    Px = Qx .* Lx;

    Px = collect(Px);
    % Px = vpa(Px); numerical approximation
    
    fprintf('P(x): %s\n', Px);

    Px_func = matlabFunction(Px);

    throttle_range = linspace(0, 100, 1000);
    interpolated_ADC = Px_func(throttle_range); 

    hold on;
    grid on;

    plot(throttle_range, interpolated_ADC, 'k-', 'LineWidth', 2);

    plot(Throttle_Percentages, ADC_Vector, 'rx', 'MarkerSize', 12, 'LineWidth', 1.5); 

    xlabel('Throttle Percentage (%)');
    ylabel('ADC Value');

    title('Barycentric Lagrange Interpolation');
    legend('Interpolated Curve', 'Data Points');

    hold off;
end
