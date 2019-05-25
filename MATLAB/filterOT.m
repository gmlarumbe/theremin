function filterOT(alpha, pl_color)

% Open Theremin filter tests
% y[n] = x[n-1] + alpha*(x[n] - x[n-1])

    b = [alpha 1-alpha];
    a = [1 0];

    %% Freq response calculations and plotting (requires a figure and hold on) - filterTests.m script

    [h w] = freqz(b,a);
    str_plot = sprintf('%s', pl_color);
    plot(w, abs(h).^2, str_plot);
    xlabel('w (rad/s)');
    ylabel('|H(w)|^2');
    str = sprintf('alpha = %f', alpha);
    title(str);

end

