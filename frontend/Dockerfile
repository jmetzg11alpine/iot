# Step 1: Use an official lightweight image for serving static files
FROM nginx:stable-alpine

RUN rm -rf /usr/share/nginx/html/*

# Step 2: Copy build files to Nginx's default directory
COPY . /usr/share/nginx/html

COPY nginx.conf /etc/nginx/conf.d/default.conf

# Step 3: Expose the default Nginx port
EXPOSE 80

# Step 4: Start Nginx
CMD ["nginx", "-g", "daemon off;"]
